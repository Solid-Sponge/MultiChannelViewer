/**
 * @file
 * @author  Michael Rossi <rossisantomauro.m@husky.neu.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 * The MultiChannelViewer is essentially the mainwindow class for the program.
 * The MultiChannelViewer is in charge of displaying the camera streams,
 * as well as displaying the GUI
 */

#ifndef MULTICHANNELVIEWER_H
#define MULTICHANNELVIEWER_H

#define WIDTH 640
#define HEIGHT 480
#define AUTOEXPOSURE_CUTOFF 3000.0

#ifdef __APPLE__
#define _OSX
#endif

#ifdef __x86_64__
#define _x64
#endif

#ifdef __i386__
#define _x86
#endif

#define ULONG_PADDING(x) (((x+3) & ~3) - x)

#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include <QLabel>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QDateTime>
#include <QImage>
#include <QPainter>
#include <QMutex>

#include <iostream>
#include <cstdlib>
#include <PvApi.h>
#include <PvRegIo.h>
#include <camera.h>
#include <FFMPEGClass.h>

namespace Ui {
class MultiChannelViewer;
}

class MultiChannelViewer : public QMainWindow
{
    Q_OBJECT

public:

    /**
     * @brief Default constructor for MultiChannelViewer object
     *
     * @param *parent Parent object (Defaults to no parent)
     */
    explicit MultiChannelViewer(QWidget *parent = 0);

    /**
      * @brief Default destructor for MultiChannelViewer object
      * Deallocates all memory
      *
      */
    ~MultiChannelViewer();


    /**
     * @brief Initializes PvAPI functions
     *
     * Also prompts users to connect two cameras if two cameras
     * aren't connected. InitializePv() will "freeze" the thread
     * if 2 cameras aren't plugged in (The GUI remains operable
     * but the program will not continue until
     * 2 cameras are plugged in).
     *
     * @return True if everything works, false otherwise
     */
    bool InitializePv();

    /**
     * @brief Automatically connects two cameras to Cam1 and Cam2
     *
     * ConnectToCam() automatically grabs the first 2 cameras it
     * sees, then assigns their handles to private members Cam1
     * and Cam2.
     *
     * @return True if everything works, false otherwise
     */
    bool ConnectToCam();    //!< TODO: Implement a way of discriminating between NIR and WL cams

    /**
     * @brief Autoexposure algorithm based on the relative intensities of pixel data
     *
     * AutoExposure() reads the latest image frames (stored in Cam1_Image and
     * Cam2_Image), and adjusts their respective cameras' exposure time. This is done
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
     */
    void AutoExposure();


signals:

    /**
     * @brief Emitted when frame from Cam1 has finished rendering
     */
    void renderFrame_Cam1_Done();

    /**
     * @brief Emitted when frame from Cam2 has finished rendering
     */
    void renderFrame_Cam2_Done();

public slots:

    /**
     * @brief Displays 24-bit RGB frame from WL Cam1 in Main GUI
     *
     * renderFrame_Cam1() is intended for the white light camera.
     * This function assumes that the original frame format is
     * 8-bit Bayer. It then automatically interpolates to 24-bit RGB
     * before displaying to screen.
     *
     * @param cam (Pointer to Cam1)
     */
    void renderFrame_Cam1(Camera *cam);

    /**
     * @brief Displays 24-bit RGB from NIR Cam2 in Main GUI
     *
     * renderFrame_Cam2() is intended for the near-infrared (NIR) camera.
     * This function assumes that the original frame format is
     * 16-bit Mono. It then applies a False coloring transformation
     * based of the intensities of the values, before displaying to
     * screen as 24-bit RGB.
     *
     * @param cam (Pointer to Cam2)
     */
    void renderFrame_Cam2(Camera *cam);

    /**
     * @brief Displays 32-bit RGBA from a combined image of WL Cam1 and NIR Cam2 in Main GUI
     *
     * renderFrame_Cam3() is intended for rendering the third screen.
     * Even though the function has Cam in it, there is no third camera attached.
     * This function renders the third screen, which is the latest NIR Cam2 image
     * on top of the latest WL Cam1 image (transparency layer).
     *
     */
    void renderFrame_Cam3();

protected:
    void closeEvent(QCloseEvent *event);

private slots:  /// GUI related functions
    void on_minVal_valueChanged(int value);

    void on_maxVal_valueChanged(int value);

    void on_minVal_sliderMoved(int position);

    void on_maxVal_sliderMoved(int position);

    void on_Record_toggled(bool checked);

    void on_Screenshot_clicked();

    void on_maxVal_spinbox_valueChanged(int arg1);

    void on_minVal_spinbox_valueChanged(int arg1);

    void on_opacitySlider_valueChanged(int value);

    void on_RegionX_WL_valueChanged(int arg1);

    void on_RegionY_WL_valueChanged(int arg1);

    void on_RegionX_NIR_valueChanged(int arg1);

    void on_RegionY_NIR_valueChanged(int arg1);

    void on_AutoExposure_stateChanged(int arg1);

    void on_Monochrome_stateChanged(int arg1);

    void on_WL_Exposure_valueChanged(int arg1);

    void on_NIR_Exposure_valueChanged(int arg1);

private:
    Ui::MultiChannelViewer *ui;
    Camera Cam1;                    //!< White Light Camera
    Camera Cam2;                    //!< Near Infrared Camera

    QImage* Cam1_Image;             //!< WL Cam frame data for third screen
    QImage* Cam2_Image;             //!< NIR Cam frame data for third screen
    unsigned char* Cam2_Image_Raw;  //!< NIR Cam raw frame date (16-bit monochrome)

    QThread thread1;                //!< WL Cam streaming thread
    QThread thread2;                //!< NIR Cam streaming thread

    QMutex Mutex1;                  //!< WL Cam Mutex (for Cam1_Image)
    QMutex Mutex2;                  //!< NIR Cam Mutex (for Cam2_Image and Cam2_Image_Raw)

    FFMPEG Video1;                  //!< WL Video Encoder
    FFMPEG Video2;                  //!< NIR Video Encoder
    FFMPEG Video3;                  //!< WL+NIR Video Encoder

    unsigned short minVal;          //!< False Coloring Minimum Threshold
    unsigned short maxVal;          //!< False Coloring Maximum Threshold
    bool recording;                 //!< Set to true when Video Encoders are recording

    bool screenshot_cam1;           //!< Set to true when screenshotting cam1
    bool screenshot_cam2;           //!< Set to true when screenshotting cam2
    bool screenshot_cam3;           //!< Set to true when screenshotting thirdscreen

    bool monochrome;                //!< If true, displays monochrome underlay in third screen
    double opacity_val;             //!< Value of opacity overlay for third screen

    bool autoexpose;                //!< If true, uses custom autoexposure algorithm
    unsigned int exposure_WL;       //!< WL cam exposure value
    unsigned int exposure_NIR;      //!< NIR cam exposure value

    int region_x_WL;                //!< X-coordinate of topleft pixel for WL cam
    int region_y_WL;                //!< Y-coordinate of topleft pixel for WL cam
    int region_x_NIR;               //!< X-coordinate of topleft pixel for NIR cam
    int region_y_NIR;               //!< Y-coordinate of topleft pixel for NIR cam
};

#endif // MULTICHANNELVIEWER_H
