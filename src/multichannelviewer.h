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

#include <PvAPI/PvApi.h>
#include <PvAPI/PvRegIo.h>

#include <camera.h>
#include <FFMPEGClass.h>
#include <autoexpose.h>

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

signals:

    /**
     * @brief Emitted when frame from Cam1 has finished rendering
     */
    void SIG_renderFrame_WL_Cam_Done();

    /**
     * @brief Emitted when frame from Cam2 has finished rendering
     */
    void SIG_renderFrame_NIR_Cam_Done();

    /**
     * @brief Emitted when Autoexposure for both cams needs to be called
     */
    void SIG_AutoExpose(QImage *WL_Image, unsigned char* NIR_Raw_Image);

    /**
     * @brief Emitted when Autoexposure for single WL cam needs to be called
     */
    void SIG_AutoExpose_WL(QImage *WL_Image);

    /**
     * @brief Emitted when Autoexposure for single NIR cam needs to be called
     */
    void SIG_AutoExpose_NIR(unsigned char* NIR_Raw_Image);

public slots:

    /**
     * @brief Displays 24-bit RGB frame from WL Cam1 in Main GUI
     *
     * renderFrame_WL_Cam() is intended for the white light camera.
     * This function assumes that the original frame format is
     * 8-bit Bayer. It then automatically interpolates to 24-bit RGB
     * before displaying to screen.
     *
     * @param cam (Pointer to Cam1)
     */
    void renderFrame_WL_Cam(Camera *cam);

    /**
     * @brief Displays 24-bit RGB from NIR Cam2 in Main GUI
     *
     * renderFrame_NIR_Cam() is intended for the near-infrared (NIR) camera.
     * This function assumes that the original frame format is
     * 16-bit Mono. It then applies a False coloring transformation
     * based of the intensities of the values, before displaying to
     * screen as 24-bit RGB.
     *
     * @param cam (Pointer to Cam2)
     */
    void renderFrame_NIR_Cam(Camera *cam);

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

    void calibrate_NIR_thresh(QAbstractButton* button);

protected:
    void closeEvent(QCloseEvent *event);

private slots:  /// GUI related functions
    void on_Record_toggled(bool checked);

    void on_Screenshot_clicked();

    void on_opacitySlider_valueChanged(int value);

    void on_RegionX_WL_valueChanged(int arg1);

    void on_RegionY_WL_valueChanged(int arg1);

    void on_RegionX_NIR_valueChanged(int arg1);

    void on_RegionY_NIR_valueChanged(int arg1);

    void on_AutoExposure_stateChanged(int arg1);

    void on_Monochrome_stateChanged(int arg1);

    void on_WL_Exposure_valueChanged(int arg1);

    void on_NIR_Exposure_valueChanged(int arg1);

    void on_actionCalibrate_NIR_triggered();

private:
    Ui::MultiChannelViewer *ui;
    Camera Cam1;                    //!< White Light Camera
    Camera Cam2;                    //!< Near Infrared Camera
    bool Two_Cameras_Connected;     //!< True if two cameras are connected, false otherwise
    bool Single_Cameras_is_WL;      //!< True if one camera is connected and is WL, false otherwise

    QImage* Cam1_Image;             //!< WL Cam frame data for third screen
    QImage* Cam2_Image;             //!< NIR Cam frame data for third screen (False colorized)
    unsigned char* Cam2_Image_Raw;  //!< NIR Cam raw frame date (16-bit monochrome)

    QThread thread1;                //!< WL Cam streaming thread
    QThread thread2;                //!< NIR Cam streaming thread

    QMutex Mutex1;                  //!< WL Cam Mutex (for Cam1_Image)
    QMutex Mutex2;                  //!< NIR Cam Mutex (for Cam2_Image and Cam2_Image_Raw)

    FFMPEG Video1;                  //!< WL Video Encoder
    FFMPEG Video2;                  //!< NIR Video Encoder
    FFMPEG Video3;                  //!< WL+NIR Video Encoder

    int thresh_calibrated;
    bool recording;                 //!< Set to true when Video Encoders are recording

    bool screenshot_cam1;           //!< Set to true when screenshotting cam1
    bool screenshot_cam2;           //!< Set to true when screenshotting cam2
    bool screenshot_cam3;           //!< Set to true when screenshotting thirdscreen

    bool monochrome;                //!< If true, displays monochrome underlay in third screen
    double opacity_val;             //!< Value of opacity overlay for third screen

    AutoExpose *exposure_control;
    bool autoexpose;                //!< If true, uses custom autoexposure algorithm
    unsigned int exposure_WL;       //!< WL cam exposure value
    unsigned int exposure_NIR;      //!< NIR cam exposure value

    int region_x_WL;                //!< X-coordinate of topleft pixel for WL cam
    int region_y_WL;                //!< Y-coordinate of topleft pixel for WL cam
    int region_x_NIR;               //!< X-coordinate of topleft pixel for NIR cam
    int region_y_NIR;               //!< Y-coordinate of topleft pixel for NIR cam

    QMessageBox* Calibrate_Window;
};

#endif // MULTICHANNELVIEWER_H
