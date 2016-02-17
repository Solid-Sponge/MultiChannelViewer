# MultiChannelViewer

## About

This program was designed for use in Medical Endoscopy. Written in QT 5.5 and C++,
it uses two AVT GigE cameras connected via a switch/router.

* **Overview**
- Supports two simultaneous video streams from AVT Gigabyte Ethernet (GigE cameras)
- One of the cameras HAS to be a White light camera (WL), and the other HAS to be a Near infrared camera (NIR)
- Supports false-coloring transformation of NIR camera, as well as adjustable false-color thresholding on-the-fly
- Supports H.264 video encoding of camera streams


* **Getting Started**
- Dependencies:
- QT 5.5 is needed to compile (Not sure about older versions)
- FFmpeg with x264 is needed to compile
- AVT's SDK (libPvAPI) is needed to compile
- With the exception of libPvAPI, all of the above can be statically compiled
- Makefile
- Use QT's QMake to generate the makefile


* **Detailed Usage**
Connect a WL and NIR AVT GigE camera via a switch/router to a single computer port. When opening the program
a message might pop up saying the program wants to use Ethernet network communications. Click on agree. After a few seconds the camera streams will pop up. The NIR camera supports false-color threshold changing on the fly, with default settings of
Minimum threshold = 0 and Maximum threshold = 4000. Increasing the minimum threshold can filter out the lower end of the NIR spectrum.
Decreasing the maximum threshold will cause the NIR camera to saturate more easily.

Clicking on the record button will enable recording. Two .avi files will be created on the root directory of the application, timestamped
and ending with _WL for white light or _NIR for near infrared. Click on record again to stop recording.

* **Developer info**
- Important Components
- main.cpp calls the main GUI and enters in an event loop
- multichannelviewer.h is the main GUI, and is in charge of displaying video streams as well as other GUI-related features
- camera.h is the container for the AVT camera. This object is meant to be placed in its own thread, and is in charge of capturing frames from the camera
- FFMPEGClass.h is the encoder. It is in charge of encoding frames that it receives from the camera
- Limitations and known issues
- Anytime the program crashes, the cameras must be unplugged and replugged back in to reset their internal memory
- Video encoding does not playback at the same framerate as the original livestream
- WL camera suffers from stuttering and fps drop, possible due to a bandwidth issue.
- If left on long enough, the WL camera heats up and displays a distorted stream with a blue-ish tint

* **Additional**
- Credits
- Rahul Sheth for creating the initial SingleChannelViewer code
- Jacob Barlow for documenting the initial SingleChannelViewer code
- AVT for their cameras and SDK
- FFmpeg for their video codecs
- Copyright and License -- License undecided as of yet.
