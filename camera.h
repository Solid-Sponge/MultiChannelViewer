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
 *
 * The Camera class is an object that represents an AVT Camera.
 * The Camera class has functions for connecting to a camera and
 * capturing frames
 */

#ifndef CAMERA_H
#define CAMERA_H

#define _OSX
#define _x64
#define FRAMESCOUNT 3

#include <QObject>
#include <QThread>
#include <QMessageBox>
#include <QApplication>
#include <PvApi.h>
#include <PvRegIo.h>
#include <cstring>
#include <iostream>

class Camera : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Default constructor for Camera object
     *
     * @param *parent Parent object (Defaults to no parent)
     */
    explicit Camera(QObject *parent = 0);

    /**
     * @brief Constructor that sets the Camera's UniqueID = the parameter passed.
     *
     * @param UniqueID Value to set Camera's UniqueID to.
     */
    Camera(unsigned long UniqueID);

    /**
     * @brief Destructor that deallocates dynamic memory
     */
    ~Camera();

    /**
     * @brief Sets Camera's UniqueID = the parameter passed
     *
     * @param ID Value to set Camera's UniqueID to.
     */
    void setID(unsigned long ID);

    /**
     * @brief Sets Camera's CameraName = the parameter passed
     *
     * @param name String(max size 32) to set CameraName to.
     */
    void setCameraName(char name[]);

    /**
     * @brief Gets the Camera's Frame (where the image data is stored)
     *
     * @return A pointer to the Camera's Frame
     */
    tPvFrame* getFramePtr();

    /**
     * @brief Assigns the Camera Handle to this object's Handle.
     *
     * Essentially establishes a link between the Camera object and the
     * physical camera itself. This function requires that the Camera's ID
     * in the private members (unsigned long ID) correspond with the physical
     * Camera's ID. The physical camera's ID can be retrieved by using the
     * PvAPI function PvCameraListEx(), then reading the Camera's ID from there.
     *
     * @return True if camera connected succesfully, false otherwise
     */
    bool GrabHandleFromID();

    /**
     * @brief Gets the camera ready for capturing and streaming images
     */
    void captureSetup();

    /**
     * @brief Stops all streaming-related processes in the camera
     */
    void captureEnd();

    /**
     * @brief Enables 16-bit Monochrome capture
     */
    void SetMono16Bit();

    /**
     * @brief Checks if camera is a White Light camera
     * @return true if camera is a White Light camera, false otherwise
     */
    bool isWhiteLight();

    /**
     * @brief Checks if camera is a Near Infrared camera
     * @return true if camera is a Near Infrared camera, false otherwise
     */
    bool isNearInfrared();

    void copyCamera(Camera &cam);

    /**
     * @brief Changes binning (resolution scaling) of camera equal to scale
     *
     * Note that the value passed scales it by 1/scale. So if a value of 2 is passed,
     * the video resolution scales down by 1/2
     */
    void changeBinning(int scale);


public slots:

    /**
     * @brief Captures a single frame in the Camera.
     *
     * The frame is then stored in tPvFrame Frames inside the object itself.
     * After it's done, it emits the frameReady signal, to be used by other functions.
     */
    void capture();

signals:

    /**
     * @brief Signal emitted after capture() has finished capturing a frame.
     * @param cam Pointer to this object
     */
    void frameReady(Camera* cam);

private:
    unsigned long   ID;                     //!< Camera's ID. Retrieved from PvCameraListEx()
    char            CameraName[32];         //!< Camera's Name. Retrieved from PvCameraListEx()
    tPvHandle       Handle;                 //!< Camera's Handle. Use GrabHandleFromID() to initialize.
    tPvFrame        Frames[FRAMESCOUNT];    //!< Camera's Frame. Use captureSetup() to initialize.
    unsigned long   FrameSize;              //!< Camera's FrameSize. Use captureSetup() to initialize.
    bool            Mono16;                 //!< Determines if Camera should operate in Mono8 or Mono16 (NIR)
    bool            Disconnected;           //!< True when Camera is disconnected
};

#endif // CAMERA_H
