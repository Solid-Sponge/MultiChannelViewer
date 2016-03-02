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

#define _OSX
#define _x64

#define ULONG_PADDING(x) (((x+3) & ~3) - x)

#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include <QLabel>
#include <QThread>
#include <QDateTime>
#include <QImage>
#include <QPainter>
#include <iostream>
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
     * on top of hte latest WL Cam1 image (transparency layer).
     *
     */
    void renderFrame_Cam3();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_minVal_valueChanged(int value);

    void on_maxVal_valueChanged(int value);

    void on_minVal_sliderMoved(int position);

    void on_maxVal_sliderMoved(int position);

    void on_Record_toggled(bool checked);

    void on_Screenshot_clicked();

    void on_checkBox_stateChanged(int arg1);

    void on_maxVal_spinbox_valueChanged(int arg1);

    void on_minVal_spinbox_valueChanged(int arg1);

    void on_opacitySlider_valueChanged(int value);

private:
    Ui::MultiChannelViewer *ui;
    Camera Cam1;                    //!< White Light Camera
    Camera Cam2;                    //!< Near Infrared Camera
    QImage* Cam1_Image;             //!< WL Cam frame data for third screen
    QImage* Cam2_Image;             //!< NIR Cam frame data for third screen
    QThread thread1;                //!< WL Cam streaming thread
    QThread thread2;                //!< NIR Cam streaming thread
    FFMPEG Video1;                  //!< WL Video Encoder
    FFMPEG Video2;                  //!< NIR Video Encoder
    FFMPEG Video3;                  //!< WL+NIR Video Encoder
    unsigned short minVal;          //!< False Coloring Minimum Threshold
    unsigned short maxVal;          //!< False Coloring Maximum Threshold
    bool recording;                 //!< Set to true when Video Encoders are recording
    bool screenshot_cam1;           //!< Set to true when screenshotting cam1
    bool screenshot_cam2;           //!< Set to true when screenshotting cam2
    bool screenshot_cam3;           //!< Set to true when screenshotting thirdscreen
    bool monochrome;
    double opacity_val;
    int region_x;
    int region_y;
};

#endif // MULTICHANNELVIEWER_H
