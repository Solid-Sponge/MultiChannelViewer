#include "autoexpose.h"

AutoExpose::AutoExpose(Camera *Cam1, Camera *Cam2, QObject *parent) : QObject(parent)
{
    this->Cam1 = Cam1;
    this->Cam2 = Cam2;
    this->exposure_WL = 80000;
    this->exposure_NIR = 300000;
}

void AutoExpose::ChangeExposure_WL(unsigned int new_exposure)
{
    this->exposure_WL = new_exposure;
}

void AutoExpose::ChangeExposure_NIR(unsigned int new_exposure)
{
    this->exposure_NIR = new_exposure;
}

void AutoExpose::AutoExposure_Two_Cams(QImage *Cam1_Image, unsigned char *Cam2_Image_Raw)
{
    /// The algorithm used here is relatively complex, and functions as a self-correcting algorithm.
    /// The first step is to acquire the latest two frames from the cameras. Since the latest frame
    /// is constantly updating, mutexes are put in place to prevent the data from updating until the
    /// frame data has been copied to local storage (freeing access to the lastest frame data as early
    /// as possible)
    ///
    /// Afterwards, the next step is to convert both frames into a quantifiable scalar format for comparison.
    /// The easiest way to do that is by comparing their intensities. The NIR image is in a 16-bit Monochrome
    /// format ranging from [0-4096], so each pixel corresponds directly to one intensity value.
    /// The WL image however, is in a 24-bit RGB format (8 bits per color) ranging from [0-255] each channel.
    /// The WL image is converted into 8-bit Monochrome format [0-256] by taking a weighted sum from each
    /// channel, keeping in mind that the human eye sees green better than red, which it sees better than blue.
    /// However, the NIR image has a higher precision (16-bit) than the new WL image (8-bit). The WL image is
    /// expanded to a domain of [0-4096] (16-bit) by multiplying each individual pixel by 16.
    ///
    /// **** Dev note: In hindsight, I'm not sure if expanding the domain is necessary. Might be removed ****
    /// **** in future                                                                                   ****
    ///
    /// Once both images are in these formats, a histogram is generated for each image. This is implemented as
    /// a simple array of 4096 elements, each index corresponding to a pixel intensity. It then goes through
    /// each pixel and increments the value at the index corresponding to the intensity value by one.
    /// (Ex: A value of 2000 in a pixel, Histogram_WL[2000]++)
    ///
    /// There's only a slight catch. When the endoscope is placed on the camera, the corners of the camera
    /// are blocked, as the endoscope has a tiny viewing space. In order to avoid counting those pixels, a
    /// mask is implemented. This mask is really simple, as it just ignores any pixels with intensities less
    /// than 32 (under the assumption that WL signal is readily available and any really low signal tends to
    /// correspond with the corners of the image). It also keeps track of the amount of pixels in the frame
    /// that correspond to actual signal data (in pixel_count), as opposed to corner pixels.
    ///
    /// As far as the NIR cam is concerned, the camera is constantly starved for signals, and the ability
    /// to distinguish between the corners of the camera and the signal itself is severely diminished as
    /// a result. The easiest remedy is to just borrow the same mask from the WL camera, under the assumption
    /// that both cameras are aligned. The histogram counts for the NIR take place in the same
    /// conditional that checks for whether a pixel is a corner-pixel or a valid one.
    ///
    /// Finally, an integral (more of a sum) is taken across both histograms up to the 95% area.
    /// What this means is each value in each index of the histogram is added up until this value reaches
    /// 95% of the valid amount of pixels (not corner pixels). When 95% is reached, the index value is recorded
    /// and the loop breaks. This value is known as the histogram cutoff. The reason 95% was chosen was to
    /// eliminate outliers at the very top due to light reflection.
    /// (Ex: Sum += Histogram[0].....Histogram[i] where i is the earliest index value that causes Sum
    ///  to be equal to 95% of pixel_count)
    ///
    /// The histogram cutoff value is compared to a constant value predefined as AUTOEXPOSURE_CUTOFF.
    /// The new exposure value is calculated based off this formula:
    ///
    ///     new_exposure = {1 - (Cutoff - AUTOEXPOSURE_CUTOFF)/ AUTOEXPOSURE_CUTOFF} * old_exposure
    ///
    /// Limits are set to ensure that the new exposure never increases or decreases by more than 30%.
    /// Additional lower and upper bounds are set individually for the WL camera and NIR camera to ensure
    /// that their corresponding exposure times never goes below or above certain values

    unsigned short* Image_WL_data = new unsigned short[HEIGHT*WIDTH];
    unsigned short* Image_NIR_data = new unsigned short[HEIGHT*WIDTH];

    //Mutex1.lock();
    //QMutexLocker *Locker = new QMutexLocker(&Mutex1);
    QImage Image_WL = Cam1_Image->copy();
    //delete Locker;
    //Mutex1.unlock();

    //Mutex2.lock();
    //Locker = new QMutexLocker(&Mutex2);
    std::memcpy(Image_NIR_data, Cam2_Image_Raw, HEIGHT*WIDTH*2);
    //delete Locker;
    //Mutex2.unlock();

    unsigned char* Image_WL_Original = Image_WL.bits();
    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        double r = Image_WL_Original[0];
        double g = Image_WL_Original[1];
        double b = Image_WL_Original[2];
        Image_WL_data[i] = static_cast<unsigned short>((0.21*r + 0.72*g + 0.07*b)*16); //!< Converts WL RGB 24-bit to 16-bit Mono
        Image_WL_Original = Image_WL_Original + 3;
    }

    int Histogram_WL[4096] = {0};
    int Histogram_NIR[4096] = {0};
    int pixel_count = 0;

    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        if (Image_WL_data[i] > 32)
        {
            unsigned short WL = static_cast<unsigned short>(Image_WL_data[i]);
            Histogram_WL[WL]++;
            unsigned short NIR = static_cast<unsigned short>(Image_NIR_data[i]);
            Histogram_NIR[NIR]++;
            pixel_count++;
        }
    }

    int Histogram_WL_integral = 0;
    int Histogram_NIR_integral = 0;
    int Histogram_WL_95percent_cutoff = 0;
    int Histogram_NIR_95percent_cutoff = 0;

    for (int i = 0; i < 4096; i++)
    {
        Histogram_WL_integral += Histogram_WL[i];
        if (Histogram_WL_integral > 0.95*pixel_count)
        {
            Histogram_WL_95percent_cutoff = i;
            break;
        }
    }
    for (int i = 0; i < 4096; i++)
    {
        Histogram_NIR_integral += Histogram_NIR[i];
        if (Histogram_NIR_integral > 0.95*pixel_count)
        {
            Histogram_NIR_95percent_cutoff = i;
            break;
        }
    }
    double exposure_WL_multiplier =
            1 - ((double) Histogram_WL_95percent_cutoff - AUTOEXPOSURE_CUTOFF)/AUTOEXPOSURE_CUTOFF;
    double exposure_NIR_multiplier =
            1 - ((double) Histogram_NIR_95percent_cutoff - AUTOEXPOSURE_CUTOFF)/AUTOEXPOSURE_CUTOFF;

    if (exposure_WL_multiplier > 1.1)
        exposure_WL_multiplier = 1.1;
    if (exposure_WL_multiplier < 0.9)
        exposure_WL_multiplier = 0.9;

    if (exposure_NIR_multiplier > 1.1)
        exposure_NIR_multiplier = 1.1;
    if (exposure_NIR_multiplier < 0.9)
        exposure_NIR_multiplier = 0.9;

    unsigned int new_exposure_WL = (double) this->exposure_WL*exposure_WL_multiplier;
    unsigned int new_exposure_NIR = (double) this->exposure_NIR*exposure_NIR_multiplier;

    if (new_exposure_WL < 100)
        new_exposure_WL = 100;
    if (new_exposure_WL > 120000) //!< 33334 to ensure >30FPS. 120000 to ensure >8FPS
        new_exposure_WL = 120000;
    //if (new_exposure_WL > 550000)
    //    new_exposure_WL = 550000;

    if (new_exposure_NIR < 100)
        new_exposure_NIR = 100;
    if (new_exposure_NIR > 550000)
        new_exposure_NIR = 550000;

    this->exposure_WL = new_exposure_WL;
    this->exposure_NIR = new_exposure_NIR;

    tPvHandle* Cam1_Handle = Cam1->getHandle();
    tPvHandle* Cam2_Handle = Cam2->getHandle();
    PvAttrUint32Set(*Cam1_Handle, "ExposureValue", this->exposure_WL);
    PvAttrUint32Set(*Cam2_Handle, "ExposureValue", this->exposure_NIR);

    delete[] Image_WL_data;
    delete[] Image_NIR_data;
}

void AutoExpose::AutoExposure_WL_Cam(QImage *Cam1_Image)
{
    unsigned short* Image_WL_data = new unsigned short[HEIGHT*WIDTH];
    QImage Image_WL = Cam1_Image->copy();

    unsigned char* Image_WL_Original = Image_WL.bits();
    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        double r = Image_WL_Original[0];
        double g = Image_WL_Original[1];
        double b = Image_WL_Original[2];
        Image_WL_data[i] = static_cast<unsigned short>((0.21*r + 0.72*g + 0.07*b)*16); //!< Converts WL RGB 24-bit to 16-bit Mono
        Image_WL_Original = Image_WL_Original + 3;
    }

    int Histogram_WL[4096] = {0};
    int pixel_count = 0;

    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        if (Image_WL_data[i] > 32)
        {
            unsigned short WL = static_cast<unsigned short>(Image_WL_data[i]);
            Histogram_WL[WL]++;
            pixel_count++;
        }
    }

    int Histogram_WL_integral = 0;
    int Histogram_WL_95percent_cutoff = 0;

    for (int i = 0; i < 4096; i++)
    {
        Histogram_WL_integral += Histogram_WL[i];
        if (Histogram_WL_integral > 0.95*pixel_count)
        {
            Histogram_WL_95percent_cutoff = i;
            break;
        }
    }
    double exposure_WL_multiplier =
            1 - ((double) Histogram_WL_95percent_cutoff - AUTOEXPOSURE_CUTOFF)/AUTOEXPOSURE_CUTOFF;

    if (exposure_WL_multiplier > 1.1)
        exposure_WL_multiplier = 1.1;
    if (exposure_WL_multiplier < 0.9)
        exposure_WL_multiplier = 0.9;

    unsigned int new_exposure_WL = (double) this->exposure_WL*exposure_WL_multiplier;

    if (new_exposure_WL < 100)
        new_exposure_WL = 100;
    if (new_exposure_WL > 120000) //!< 33334 to ensure >30FPS. 120000 to ensure >8FPS
        new_exposure_WL = 120000;

    this->exposure_WL = new_exposure_WL;

    tPvHandle* Cam1_Handle = Cam1->getHandle();
    PvAttrUint32Set(*Cam1_Handle, "ExposureValue", this->exposure_WL);

    delete[] Image_WL_data;
}
