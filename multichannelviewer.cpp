#include "multichannelviewer.h"
#include "ui_multichannelviewer.h"
#include "qapplication.h"

//#define minVal 0
//#define maxVal 4000


MultiChannelViewer::MultiChannelViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MultiChannelViewer)
{
    ui->setupUi(this);
    this->show();   //!< Displays main GUI


    /// Setting initial values for recording, screenshot, and False-coloring thresholds
    minVal = 0;
    maxVal = 4000;
    recording = false;
    screenshot_cam1 = false;
    screenshot_cam2 = false;
    monochrome = false;
    opacity_val = 0.1;
    ui->minVal->setRange(0,4095);
    ui->maxVal->setRange(0,4095);
    ui->minVal->setValue(10);
    ui->maxVal->setValue(200);
    autoexpose = true;
    exposure_WL = 60000;
    exposure_NIR = 500000;
    Cam1_Image = new QImage(WIDTH, HEIGHT, QImage::Format_RGB888);
    Cam2_Image = new QImage(WIDTH, HEIGHT, QImage::Format_RGB888);
    Cam2_Image_Raw = new unsigned char[HEIGHT*WIDTH*2];
    Cam1_Image->fill(0);
    Cam2_Image->fill(0);


    if (this->InitializePv() && this->ConnectToCam()) //!< Executes if PvAPI initializes and Cameras connect successfully
    {
        Cam1.moveToThread(&thread1); //!< Assigns Cam1 to Thread1
        Cam2.moveToThread(&thread2); //!< Assigns Cam2 to Thread2

        Cam2.SetMono16Bit();    //!< Enables 16-bit Mono format for Cam2 (NIR cam)


        /// Connects various slots and signals. Identical for the second set of connections
        ///
        /// The first connection connects the frameReady signal to the renderFrame slot.
        /// When the camera has finished capturing a frame, the renderFrame_Cam1 function will execute
        /// displaying the frame on the Main GUI window
        ///
        /// The second connection connects the renderFrame_Cam1_Done signal to the capture() slot.
        /// This completes the loop so that when the Main GUI has finished displaying the frame,
        /// it tells the camera to capture another frame
        ///
        /// The third connection ties the camera object to its own thread, and by extension, tying all these
        /// connections to that same thread, so that the camera captures and displays an image all on its own
        /// thread
        connect(&Cam1, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_Cam1(Camera*)),Qt::AutoConnection);
        connect(this, SIGNAL(renderFrame_Cam1_Done()), &Cam1, SLOT(capture()),Qt::AutoConnection);
        connect(&thread1, SIGNAL(started()), &Cam1, SLOT(capture()));


        ///Identical to the above set of slots and signals, except for Cam2 and Cam2-specific functions
        connect(&Cam2, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_Cam2(Camera*)),Qt::AutoConnection);
        connect(this, SIGNAL(renderFrame_Cam2_Done()), &Cam2, SLOT(capture()), Qt::AutoConnection);
        connect(&thread2, SIGNAL(started()), &Cam2, SLOT(capture()));

        Cam1.captureSetup();    //!< Sets up Cam1 capture settings
        Cam2.captureSetup();    //!< Sets up Cam2 capture settings

        thread1.start();        //!< Initializes thread1 and starts Cam1 stream/display
        thread2.start();        //!< Initializes thread2 and starts Cam2 stream/display

    }
    else
    {
        std::cout << "InitalizePv or ConnectToCam Failed" << std::endl;
        QMessageBox errBox;
        errBox.critical(0,"Error","Could not connect to cameras. Please unplug cameras and try again.");
        errBox.setFixedSize(500,200);
        QApplication::exit(1);
    }
}

MultiChannelViewer::~MultiChannelViewer()
{
    delete ui;
}

bool MultiChannelViewer::InitializePv()
{
    tPvErr errcodeinit = PvInitialize(); //Initialize PvAPI module
    if (errcodeinit != ePvErrSuccess) //Program shuts down if weird error is encountered
    {
        std::cout << "PvInitialize err: " << errcodeinit << std::endl;
        QMessageBox errBox;
        errBox.critical(0,"Error","Camera Module could not be initalized.");
        errBox.setFixedSize(500,200);
        return false;
    }

    QThread::sleep(3);
    if (PvCameraCount() < 2)
    {
        QMessageBox errBox;
        errBox.critical(0,"Error","2 Cameras not found. Please plug in 2 cameras then press Ok to continue.");
        errBox.setFixedSize(500,200);
    }
    while (PvCameraCount() < 2)
    {
        QThread::msleep(100);
        qApp->processEvents();
    }

    std::cout << "Cameras Found: " << PvCameraCount() << std::endl;
    return true;
}

bool MultiChannelViewer::ConnectToCam()
{
    tPvCameraInfoEx   info[2];
    unsigned long     numCameras;
    numCameras = PvCameraListEx(info, 2, NULL, sizeof(tPvCameraInfoEx));
    // Open the first two cameras found, if it’s not already open.
    if ((numCameras >= 1) && (info[0].PermittedAccess & ePvAccessMaster) && (info[1].PermittedAccess & ePvAccessMaster))
    {
        Cam1.setID(info[0].UniqueId);
        Cam1.setCameraName(info[0].CameraName);

        Cam2.setID(info[1].UniqueId);
        Cam2.setCameraName(info[1].CameraName);

        if (Cam1.GrabHandleFromID() && Cam2.GrabHandleFromID())
        {
            if (!Cam1.isWhiteLight() && !Cam2.isNearInfrared())
            {
                Camera temp;
                temp.copyCamera(Cam1);
                Cam1.copyCamera(Cam2);
                Cam2.copyCamera(temp);
            }

            return true;
        }
        return false;
    }
    return false;
}

void MultiChannelViewer::AutoExposure()
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

    if (!autoexpose)
        return;

    unsigned short* Image_WL_data = new unsigned short[HEIGHT*WIDTH];
    unsigned short* Image_NIR_data = new unsigned short[HEIGHT*WIDTH];

    Mutex1.lock();
    QImage Image_WL = Cam1_Image->copy();
    Mutex1.unlock();

    Mutex2.lock();
    //QImage Image_NIR = Cam2_Image->copy();
    std::memcpy(Image_NIR_data, Cam2_Image_Raw, HEIGHT*WIDTH*2);
    Mutex2.unlock();

    unsigned char* Image_WL_Original = Image_WL.bits();
    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        double r = Image_WL_Original[0];
        double g = Image_WL_Original[1];
        double b = Image_WL_Original[2];
        Image_WL_data[i] = (0.21*r + 0.72*g + 0.07*b)*16; //!< Converts WL RGB 24-bit to 16-bit Mono
        Image_WL_Original = Image_WL_Original + 3;
    }

    int Histogram_WL[4096] = {0};
    int Histogram_NIR[4096] = {0};
    int pixel_count = 0;

    for (int i = 0; i < Image_WL.height()*Image_WL.width(); i++)
    {
        if (Image_WL_data[i] > 32)
        {
            Histogram_WL[Image_WL_data[i]]++;
            Histogram_NIR[Image_NIR_data[i]]++;
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
    if (new_exposure_WL > 120000)
        new_exposure_WL = 120000;
    //if (new_exposure_WL > 550000)
    //    new_exposure_WL = 550000;

    if (new_exposure_NIR < 100)
        new_exposure_NIR = 100;
    if (new_exposure_NIR > 550000)
        new_exposure_NIR = 550000;

    this->exposure_WL = new_exposure_WL;
    this->exposure_NIR = new_exposure_NIR;

    tPvHandle* Cam1_Handle = Cam1.getHandle();
    tPvHandle* Cam2_Handle = Cam2.getHandle();
    PvAttrUint32Set(*Cam1_Handle, "ExposureValue", this->exposure_WL);
    PvAttrUint32Set(*Cam2_Handle, "ExposureValue", this->exposure_NIR);

    //if (new_exposure_WL)
    delete[] Image_WL_data;
    delete[] Image_NIR_data;
}


void MultiChannelViewer::renderFrame_Cam1(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();
    //PvAttrUint32Set(*CamHandle, "ExposureValue", 15000);

    unsigned long line_padding = ULONG_PADDING(FramePtr1->Width*3);
    unsigned long line_size = (FramePtr1->Width*3) + line_padding;
    unsigned long buffer_size = line_size * FramePtr1->Height;
    unsigned char* buffer = new unsigned char[buffer_size];
    unsigned char* bufferPtr = buffer;

    /// RGB frame data from camera is in Bayer 8-bit format. This function converts it to RGB 24-bit.
    PvUtilityColorInterpolate(FramePtr1, &buffer[0], &buffer[1], &buffer[2], 2, 0);

    QImage imgFrame(FramePtr1->Width, FramePtr1->Height, QImage::Format_RGB888);

    /// Sets corresponding pixels of imgFrame equal to the camera frame data.
    /// Uses pointer arithmetic to traverse through the frame data.
    for (int i = 0; i < FramePtr1->Height; i++)
    {
        for (int j = 0; j < FramePtr1->Width; j++)
        {
            unsigned char r = *bufferPtr;   //!< R-Pixel
            bufferPtr++;
            unsigned char g = *bufferPtr;   //!< G-Pixel
            bufferPtr++;
            unsigned char b = *bufferPtr;   //!< B-Pixel
            bufferPtr++;
            QRgb color = qRgb(r, g, b);
            imgFrame.setPixel(j, i, color); //!< Sets pixel with (r,g,b) at location j,i
        }
    }
    imgFrame = imgFrame.mirrored(true, false);  //!< Flips WL image. Needed due to dicroic
    ui->cam_1->setScaledContents(true);
    ui->cam_1->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_1->show(); //!< Displays image on Main GUI

    Mutex1.lock();
    *Cam1_Image = imgFrame; //!< Updates latest frame. Mutex in place to prevent race conditions
    Mutex1.unlock();

    qApp->processEvents(); //!< Process other GUI events

    if (screenshot_cam1) //!< Takes screenshot
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL.png");
        //timestamp = QString("/Screenshot/") + timestamp;

        QFile file(timestamp);
        //QFile file(QCoreApplication::applicationDirPath() + "/Screenshot/" + timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam1 = false;
    }
    if (recording) //!< Starts recording
    {
        //unsigned char* mirror_buffer = new unsigned char[buffer_size];
        //unsigned char* mirror_buffer_Ptr = mirror_buffer;
        unsigned char* mirror_buffer;
        mirror_buffer = imgFrame.bits();
        //Video1.WriteFrame(buffer);
        Video1.WriteFrame(mirror_buffer);
    }

    renderFrame_Cam3(); //!< Renders third screen on GUI

    delete[] buffer;
    emit renderFrame_Cam1_Done(); //!< Tells WL camera to capture another frame
}

void MultiChannelViewer::renderFrame_Cam2(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();
    PvAttrUint32Set(*CamHandle, "ExposureValue", this->exposure_NIR);

    unsigned short* rawPtr = static_cast<unsigned short*>(FramePtr1->ImageBuffer);

    /// False coloring. Sets up 7 thresholds divided evenly between minVal and maxVal
    /// Displays a particular RGB value that corresponds to the intensity of the pixels
    int thresh1,thresh2,thresh3,thresh4,thresh5,thresh6,thresh7;
    thresh1 = minVal;
    thresh7 = maxVal;
    int space = (thresh7 - thresh1)/6;
    thresh2 = thresh1 + space;
    thresh3 = thresh2 + space;
    thresh4 = thresh3 + space;
    thresh5 = thresh4 + space;
    thresh6 = thresh5 + space;


    unsigned char* buffer = new unsigned char[3*FramePtr1->Height*FramePtr1->Width];
    unsigned char* bufferPtr = buffer;

    QImage imgFrame(FramePtr1->Width, FramePtr1->Height, QImage::Format_RGB888);
    for (int i = 0; i < FramePtr1->Height; i++)
    {
        for (int j = 0; j < FramePtr1->Width; j++)
        {
            unsigned short counts = *rawPtr;
            //unsigned char counts = *rawPtr;
            unsigned char r;
            unsigned char g;
            unsigned char b;
            if (counts <= thresh1)
            {
                r = 0;
                g = 0;
                b = 0;
            }
            else if (counts <= thresh2)
            {
                r = 0;
                g = 0;
                b = 255;
            }
            else if (counts <= thresh3)
            {
                r = 0;
                g = 255;
                b = 255;
            }
            else if (counts <= thresh4)
            {
                r = 0;
                g = 255;
                b = 255*(1-(counts-thresh3)/space);
            }
            else if (counts <= thresh5)
            {
                r = 255*(counts-thresh4)/space;
                g = 255;
                b = 0;
            }
            else if (counts <= thresh6)
            {
                r = 255;
                g = 255*(1-(counts-thresh5)/space);
                b = 0;
            }
            else if (counts <= thresh7)
            {
                r = 255;
                g = 255*(counts-thresh6)/space;
                b = 255*(counts-thresh6)/space;
            }
            else
            {
                r = 255;
                g = 255;
                b = 255;
            }

            bufferPtr[0] = r;
            bufferPtr[1] = g;
            bufferPtr[2] = b;
            bufferPtr = bufferPtr + 3;

            QRgb color = qRgb(r, g, b);
            imgFrame.setPixel(j, i, color);
            rawPtr++;
        }
    }
    ui->cam_2->setScaledContents(true);
    ui->cam_2->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_2->show();


   // Mutex2.lock();
    *Cam2_Image = imgFrame;
  //  Mutex2.unlock();

    Mutex2.lock();
    std::memcpy(Cam2_Image_Raw, FramePtr1->ImageBuffer, FramePtr1->ImageBufferSize);
    Mutex2.unlock();

    qApp->processEvents();

    if (screenshot_cam2)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_NIR.png");

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam2 = false;
    }

    if (recording)
    {
        Video2.WriteFrame(buffer);
    }

    //renderFrame_Cam3();


    delete[] buffer;

    emit renderFrame_Cam2_Done();
}

void MultiChannelViewer::renderFrame_Cam3()
{
    QImage Cam1_Image_Mono;
    QPixmap imgFrame(Cam1_Image->size());
    QPainter p(&imgFrame);

    Mutex1.lock();
    if (monochrome)
    {
        Cam1_Image_Mono = Cam1_Image->copy();
        unsigned char *data = Cam1_Image_Mono.bits();
        int pixelCount = Cam1_Image_Mono.width() * Cam1_Image_Mono.height();

        // Convert each pixel to grayscale
        for(int i = 0; i < pixelCount; ++i)
        {
           int r = data[0];
           int g = data[1];
           int b = data[2];
           int val = qGray(r, g, b); //!< Qt's integrated Grayscale conversion

           data[0] = val;
           data[1] = val;
           data[2] = val;
           data += 3;
        }
        p.drawImage(QPoint(0,0), Cam1_Image_Mono);
    }
    else
    {
        p.drawImage(QPoint(0,0), *Cam1_Image);
    }
    Mutex1.unlock();

    Mutex2.lock();
    unsigned char* cam2_data = Cam2_Image->bits();
    QImage Cam2_Image_transparency = Cam2_Image->convertToFormat(QImage::Format_ARGB32);
    QRgb transparent_pixel = qRgba(0,0,0,0);

    for (int i = 0; i < Cam2_Image->height(); i++)
    {
        for (int j = 0; j < Cam2_Image->width(); j++)
        {
            if (cam2_data[0] == 0 && cam2_data[1] == 0 && cam2_data[2] == 0)
            {
                Cam2_Image_transparency.setPixel(j,i,transparent_pixel);
            }
            else
            {
                QRgb pixel = qRgba(cam2_data[0],cam2_data[1],cam2_data[2],255*opacity_val);
                Cam2_Image_transparency.setPixel(j,i,pixel);
            }
            cam2_data = cam2_data + 3;
        }
    }
    Mutex2.unlock();

    p.drawImage(QPoint(0,0), Cam2_Image_transparency);
    p.end();

    ui->cam_3->setScaledContents(true);
    ui->cam_3->setPixmap(imgFrame);
    ui->cam_3->show();

    qApp->processEvents();

    if (autoexpose)
        QtConcurrent::run(this, &MultiChannelViewer::AutoExposure);

    if (screenshot_cam3)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL+NIR.png");

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam3 = false;
    }

    if (recording)
    {
        QImage RGB24 = imgFrame.toImage();
        RGB24 = RGB24.convertToFormat(QImage::Format_RGB888);
        Video3.WriteFrame(RGB24.bits());
    }
}

void MultiChannelViewer::closeEvent(QCloseEvent *event)
{
    if (recording)
    {
        Video1.CloseVideo();
        Video2.CloseVideo();
        Video3.CloseVideo();
    }

    QThread::sleep(1);

    thread1.quit();
    thread2.quit();

    Cam1.captureEnd();
    Cam2.captureEnd();
    delete[] Cam2_Image_Raw;

    PvUnInitialize();
    QApplication::exit(0);
}


void MultiChannelViewer::on_minVal_valueChanged(int value)
{
    if (value < this->maxVal - 100)
    {
        this->minVal = value;
        ui->minVal_spinbox->setValue(value);

    }
    else
    {
        this->minVal = this->maxVal - 100;
        ui->minVal_spinbox->setValue(this->maxVal - 100);
    }
}

void MultiChannelViewer::on_maxVal_valueChanged(int value)
{
    if (value > this->minVal + 100)
    {
        this->maxVal = value;
        ui->maxVal_spinbox->setValue(value);
    }
    else
    {
        this->maxVal = this->minVal + 100;
        ui->maxVal_spinbox->setValue(this->minVal + 100);
    }
}

void MultiChannelViewer::on_minVal_spinbox_valueChanged(int arg1)
{
    if (arg1 < this->maxVal - 100)
    {
        this->minVal = arg1;
        ui->minVal_spinbox->setValue(arg1);
        ui->minVal->setValue(arg1);
    }
    else
    {
        this->minVal = this->maxVal - 100;
        ui->minVal_spinbox->setValue(this->maxVal - 100);
        ui->minVal->setValue(this->maxVal - 100);
    }
}

void MultiChannelViewer::on_maxVal_spinbox_valueChanged(int arg1)
{
    if (arg1 > this->minVal + 100)
    {
        this->maxVal = arg1;
        ui->maxVal_spinbox->setValue(arg1);
        ui->maxVal->setValue(arg1);
    }
    else
    {
        this->maxVal = this->minVal + 100;
        ui->maxVal_spinbox->setValue(this->minVal + 100);
        ui->maxVal->setValue(this->minVal + 100);
    }
}

void MultiChannelViewer::on_minVal_sliderMoved(int position)
{
    if (position > this->maxVal - 100)
    {
        ui->minVal_spinbox->setValue(position);
    }
}

void MultiChannelViewer::on_maxVal_sliderMoved(int position)
{
    if (position > this->minVal + 100)
    {
        ui->maxVal_spinbox->setValue(position);
    }
}

void MultiChannelViewer::on_Record_toggled(bool checked)
{
    if (checked)
    {
        QString timestamp_filename_WL = QDateTime::currentDateTime().toString();
        timestamp_filename_WL.append("_WL.avi");
        timestamp_filename_WL.replace(QString(" "), QString("_"));
        timestamp_filename_WL.replace(QString(":"), QString("-"));
        timestamp_filename_WL = QString("Video/") + timestamp_filename_WL;

        QString timestamp_filename_NIR = QDateTime::currentDateTime().toString();
        timestamp_filename_NIR.append("_NIR.avi");
        timestamp_filename_NIR.replace(QString(" "), QString("_"));
        timestamp_filename_NIR.replace(QString(":"), QString("-"));

        QString timestamp_filename_WL_NIR = QDateTime::currentDateTime().toString();
        timestamp_filename_WL_NIR.append("_WL+NIR.avi");
        timestamp_filename_WL_NIR.replace(QString(" "), QString("_"));
        timestamp_filename_WL_NIR.replace(QString(":"), QString("-"));

        char* filename_WL = (char*) timestamp_filename_WL.toStdString().c_str();
        this->Video1.SetupVideo(filename_WL, 640, 480, 15, 2, 750000); //bitrate = 40000000

        char* filename_NIR = (char*) timestamp_filename_NIR.toStdString().c_str();
        this->Video2.SetupVideo(filename_NIR, 640, 480, 15, 2, 750000);

        char* filename_WL_NIR = (char*) timestamp_filename_WL_NIR.toStdString().c_str();
        this->Video3.SetupVideo(filename_WL_NIR, 640, 480, 15, 2, 750000);

        recording = true;
    }
    if (!checked)
    {
        recording = false;
        Video1.CloseVideo();
        Video2.CloseVideo();
        Video3.CloseVideo();
    }
}

void MultiChannelViewer::on_Screenshot_clicked()
{
    this->screenshot_cam1 = true;
    this->screenshot_cam2 = true;
    this->screenshot_cam3 = true;
}

void MultiChannelViewer::on_Monochrome_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked)
        this->monochrome = true;
    if (arg1 == Qt::Unchecked)
        this->monochrome = false;
}

void MultiChannelViewer::on_opacitySlider_valueChanged(int value)
{
    opacity_val = static_cast<double>(value) / 10.0;
}

void MultiChannelViewer::on_RegionX_WL_valueChanged(int arg1)
{
    PvAttrUint32Set(*(Cam1.getHandle()), "RegionX", arg1);
}

void MultiChannelViewer::on_RegionY_WL_valueChanged(int arg1)
{
    PvAttrUint32Set(*(Cam1.getHandle()), "RegionY", arg1);
}

void MultiChannelViewer::on_RegionX_NIR_valueChanged(int arg1)
{
    PvAttrUint32Set(*(Cam2.getHandle()), "RegionX", arg1);
}

void MultiChannelViewer::on_RegionY_NIR_valueChanged(int arg1)
{
    PvAttrUint32Set(*(Cam2.getHandle()), "RegionY", arg1);
}

void MultiChannelViewer::on_AutoExposure_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked)
    {
        this->autoexpose = true;
        ui->WL_Exposure->setReadOnly(true);
        ui->NIR_Exposure->setReadOnly(true);
    }
    if (arg1 == Qt::Unchecked)
    {
        this->autoexpose = false;
        ui->WL_Exposure->setReadOnly(false);
        ui->NIR_Exposure->setReadOnly(false);
    }
}

void MultiChannelViewer::on_WL_Exposure_valueChanged(int arg1)
{
    this->exposure_WL = static_cast<unsigned int>(arg1);
    tPvHandle *cam = Cam1.getHandle();
    if (cam != NULL)
        PvAttrUint32Set(*cam, "ExposureValue", arg1);
}

void MultiChannelViewer::on_NIR_Exposure_valueChanged(int arg1)
{
    this->exposure_NIR = static_cast<unsigned int>(arg1);
    tPvHandle *cam = Cam2.getHandle();
    if (cam != NULL)
        PvAttrUint32Set(*cam, "ExposureValue", arg1);
}
