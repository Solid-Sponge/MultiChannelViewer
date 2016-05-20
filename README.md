# MultiChannelViewer

## About

This program was designed for use in Medical Endoscopy. Written in QT 5.5 and C++,
it uses up to two AVT GigE cameras connected via a switch/router.

**Overview**
- Supports up to two simultaneous video streams from AVT Gigabyte Ethernet (GigE cameras)
- White-light (WL) camera, Near-Infrared (NIR) camera, or both supported
- Supports false-coloring rendering of NIR camera, as well as auto-adjustable false-color thresholding with calibration
- Supports timestamped H.264 video encoding of camera streams, as well as timestamped screenshots


**Getting Started**
Dependencies:
- QT 5.5 is needed to compile (Not sure about older versions)
- FFmpeg with x264 is needed to compile *(included in src/lib)*
- AVT's SDK (libPvAPI) is needed to compile *(included in src/lib)*
- Use QT's QMake to generate the makefile


**Detailed Usage**
Connect a WL and/or NIR AVT GigE camera via a switch/router to a single computer port. When opening the program a list of cameras will show up, with the option to refresh cameras or continue ahead with selected cameras. A message may additionally pop up asking to use Ethernet network communications. Agree to this. After a few seconds the camera streams will pop up. 

Depending on how many cameras are hooked up, you may see a WL image, NIR image, or 3 images (WL, NIR, WL+NIR) if both cameras are connected. Different camera configurations result in different UI's, with different settings. Each camera has an exposure value and coordinate values (RegionX and RegionY). In the future, coordinate values may be loaded/saved for different configurations.

NIR camera has a special auto-thresholding system in place. Clicking on Options->Calibrate NIR will prompt the user to point the NIR camera in a "signal-less" area (background noise), then click OK. This will then take the average value of the entire NIR frame and set that as the new minimum threshold. Every pixel that corresponds to a value less than or equal to that new minimum threshold will be colored black, essentially removing noise. The rest of the signal is split into 6 colors (Blue, Cyan, Green, Yellow, Red, White; from weakest to strongest). This split is based off a percentage using a histogram, rather than an arbitrary cutoff. This ensures the use of the full color spectrum as opposed to just one or two colors for extremely weak/strong signals.

Clicking on screenshot will take a screenshot of the immediate frame onscreen. Three .png files will be created under a new folder in the root directory of the program "Screenshots". The screenshots are timestamped and end with _WL, _NIR, or _WL+NIR.
 
Clicking on the record button will enable recording. Three .avi files will be created under a new folder on the root directory of the application "Video",. The videos are timestamped and end with _WL, _NIR, or _WL+NIR. Click on record again to stop recording.

**Developer info**
*Important Components*
- main.cpp calls the main GUI and enters in an event loop
- multichannelviewer.h is the main GUI, and is in charge of displaying video streams as well as other GUI-related features
- camera.h is the container for the AVT camera. This object is meant to be placed in its own thread, and is in charge of capturing frames from the camera
- FFMPEGClass.h is the encoder. It is in charge of encoding frames that it receives from the camera
- Autoexposure.h controls everything related to autoexposure. Runs in its own thread.
- Limitations and known issues
- Anytime the program crashes, the cameras must be unplugged and replugged back in to reset their internal memory
- Video encoding does not playback at the same framerate as the original livestream
- WL camera suffers from stuttering and fps drop, possible due to a bandwidth issue.
- If left on long enough, the WL camera heats up and displays a distorted stream with a blue-ish tint

 **Additional**
*Credits*
- Rahul Sheth for creating the initial SingleChannelViewer code
- Jacob Barlow for documenting the initial SingleChannelViewer code
- Umar Mahmood for outlining and specifying key features as well as implementation
- Pedram Heidari for specfying key features, algorithmic implementation, as well as debugging.
- AVT for their cameras and SDK
- FFmpeg for their video codecs

**Copyright and License** -- GNU GPL v3