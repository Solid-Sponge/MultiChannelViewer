#ifndef AUTOEXPOSE_H
#define AUTOEXPOSE_H

#define WIDTH 640
#define HEIGHT 480
#define AUTOEXPOSURE_CUTOFF 3000.0

#include <QObject>
#include <camera.h>

class AutoExpose : public QObject
{
    Q_OBJECT
public:
    explicit AutoExpose(Camera* Cam1 = 0, Camera* Cam2 = 0, QObject *parent = 0);
    void ChangeExposure_WL(unsigned int new_exposure);
    void ChangeExposure_NIR(unsigned int new_exposure);

signals:

public slots:

    /**
     * @brief Autoexposure algorithm based on the relative intensities of pixel data
     *
     * AutoExposure_Two_Cams() reads the latest image frames (stored in Cam1_Image and
     * Cam2_Image_Raw), and adjusts their respective cameras' exposure time. This is done
     * by attempting to map 95% of pixel intensity at an exact threshold. If it detects
     * that the overall intensity is higher than the threshold, the exposure is reduced,
     * and if the overall intensity is lower than the threshold, the exposure is increased.
     *
     * Min and Max caps are set to prevent integrating for too short or too long. Additionally,
     * a check is placed to prevent the exposure time from increasing more or less than 30% of
     * the previous value.
     *
     * This function is thread-safe, and in fact is recommended to be run in a seperate thread.
     * The latest frame datas have mutex locks to prevent the data from being overwritten while
     * copying the data to stack memory.
     *
     * Currently, the NIR camera tends to max out the exposure time as it is constantly signal-starved.
     * The WL camera sits at a comfortable frame-rate, but the exposure time usually ends up a bit lower
     * than it could be, resulting in a slightly starved signal from distances,
     *
     * @param Cam1_Image Latest WL image. Function makes an internal copy.
     * @param Cam2_Image_Raw Latest NIR-image (raw, not false-colored). Function makes an internal copy.
     */
    void AutoExposure_Two_Cams(QImage* Cam1_Image, unsigned char* Cam2_Image_Raw);

    /**
     * @brief Autoexposure algorithm based on the relative intensities of pixel data for WL Camera only
     *
     * AutoExposure_WL_Cam operates the same way as AutoExposure_Two_Cams does, but only for the WL camera.
     *
     * @param Cam1_Image Latest WL image. Function makes an internal copy.
     */
    void AutoExposure_WL_Cam(QImage* Cam1_Image);

    void AutoExposure_NIR_Cam(unsigned char* Cam2_Image_Raw);

private:
    Camera* Cam1;
    Camera* Cam2;
    unsigned int exposure_WL;
    unsigned int exposure_NIR;
};

#endif // AUTOEXPOSE_H
