#include "camera.h"

Camera::Camera(QObject *parent) : QObject(parent)
{
    this->Mono16 = false;
    this->Disconnected = false;
    this->Handle = NULL;
}

Camera::Camera(unsigned long UniqueID)
{
    tPvErr err = PvCameraOpen(UniqueID, ePvAccessMaster, &Handle);
    this->Mono16 = false;
    this->Disconnected = false;
}

Camera::~Camera()
{
   // for (int i = 0; i < FRAMESCOUNT; i++)
        delete[] static_cast<unsigned char*>(Frames[0].ImageBuffer);
}

void Camera::setID(unsigned long ID)
{
    this->ID = ID;
}

void Camera::setCameraName(char name[])
{
    std::strncpy(this->CameraName, name, 32);
}

tPvFrame* Camera::getFramePtr()
{
    return &(this->Frames[0]);
}

tPvHandle* Camera::getHandle()
{
    return &(this->Handle);
}

bool Camera::GrabHandleFromID()
{
    tPvErr err = PvCameraOpen(this->ID, ePvAccessMaster, &Handle);
    if (err != ePvErrSuccess)
        return false;
    return true;
}

void Camera::captureSetup()
{
    if (this->Mono16)
    {
        PvAttrEnumSet(this->Handle, "PixelFormat", "Mono16");   //!< Sets frame format to Mono 16-bit if Mono16 = true
        //PvAttrBooleanSet(this->Handle, "StreamFrameRateConstrain", true);
        PvAttrEnumSet(this->Handle, "ExposureMode", "Manual");
        PvAttrUint32Set(this->Handle, "ExposureValue", 500000);
        PvAttrUint32Set(this->Handle,"StreamBytesPerSecond", 115000000 / 2 - 20000000);

        PvAttrUint32Set(this->Handle, "BinningX", 1);
        PvAttrUint32Set(this->Handle, "BinningY", 1);

        PvAttrUint32Set(this->Handle, "Width", 640);
        PvAttrUint32Set(this->Handle, "Height", 480);
        PvAttrUint32Set(this->Handle, "RegionX", 388);  //x = 400
        PvAttrUint32Set(this->Handle, "RegionY", 250);  //y = 250
    }
    else
    {
        PvAttrUint32Set(this->Handle, "StreamBytesPerSecond", 115000000 / 2 + 20000000);
        PvAttrEnumSet(this->Handle, "PixelFormat", "Bayer8");

        PvAttrUint32Set(this->Handle, "BinningX", 1);
        PvAttrUint32Set(this->Handle, "BinningY", 1);

        PvAttrUint32Set(this->Handle, "Width", 640); //x = 269
        PvAttrUint32Set(this->Handle, "Height", 480); //y = 332
        PvAttrUint32Set(this->Handle, "RegionX", 450);
        PvAttrUint32Set(this->Handle, "RegionY", 180);

        //PvAttrUint32Set(this->Handle, "ExposureAutoMax", 60000);
        PvAttrEnumSet(this->Handle, "ExposureMode", "Manual");
        PvAttrUint32Set(this->Handle, "ExposureValue", 60000);

    }

    tPvErr errCode;
    if((errCode = PvAttrUint32Get(this->Handle,"TotalBytesPerFrame",&this->FrameSize)) != ePvErrSuccess)
    {
        std::cout << "Error reading from cam.";
        PvCameraClose(this->Handle);
        return;
    }
    memset(&(this->Frames[0]),0,sizeof(tPvFrame));

    this->Frames[0].ImageBuffer = new char[this->FrameSize];
    this->Frames[0].ImageBufferSize = this->FrameSize;

    PvCaptureAdjustPacketSize(this->Handle,8228);
   // PvAttrUint32Set(this->Handle,"StreamBytesPerSecond", 115000000 / 2);

    //start driver stream
    PvCaptureStart(this->Handle);
    //set frame triggers to be generated internally
    PvAttrEnumSet(this->Handle, "FrameStartTriggerMode", "Freerun");
    //set camera to receive continuous number of frame triggers
    PvAttrEnumSet(this->Handle, "AcquisitionMode", "Continuous");
    //PvCaptureAdjustPacketSize(this->Handle,8228);
    PvAttrUint32Set(this->Handle, "HeartbeatTimeout", 775000);

    tPvUint32 interval;
    PvAttrUint32Get(this->Handle, "HeartbeatInterval", &interval);
    std::cout << std::endl << interval << " interval" << std::endl;

    char PartVer[50];
    PvAttrStringGet(this->Handle, "PartVersion",PartVer,50, NULL);
    unsigned long PartNum;
    PvAttrUint32Get(this->Handle, "PartNumber", &PartNum);

    std::cout << "PartVer: " << PartVer << "\nPartNum: " << PartNum << std::endl;
}

void Camera::captureEnd()
{
    if (!this->Disconnected)
    {
        PvCommandRun(this->Handle, "AcquisitionStop");
        PvCaptureEnd(this->Handle);
        PvCameraClose(this->Handle);
    }
}

void Camera::capture()
{

    PvCommandRun(this->Handle, "AcquisitionStart");
    //PvCaptureQueueClear(this->Handle);

    tPvErr errcode = PvCaptureQueueFrame(this->Handle, &(this->Frames[0]), NULL);
    if (errcode != ePvErrSuccess)
        std::cout << "Frame is Kill" << std::endl;
    if (errcode == ePvErrUnplugged)
    {
        this->Disconnected = true;
        QMessageBox errBox;
        errBox.critical(0,"Error","Camera unplugged. Please replug camera and restart program.");
        errBox.setFixedSize(500,200);
        QApplication::exit(1);
    }

    PvCaptureWaitForFrameDone(this->Handle, &(this->Frames[0]), PVINFINITE);
    PvCommandRun(this->Handle, "AcquisitionStop");

    emit frameReady(this);
}

void Camera::SetMono16Bit()
{
    this->Mono16 = true;
}

bool Camera::isWhiteLight()
{
    char PartVer[20];
    PvAttrStringGet(this->Handle, "PartVersion",PartVer, 20, NULL);
    unsigned long PartNum;
    PvAttrUint32Get(this->Handle, "PartNumber", &PartNum);

    if ((std::strcmp(PartVer, "A") == 0) && PartNum == 20007)
        return true;
    return false;
}

bool Camera::isNearInfrared()
{
    char PartVer[20];
    PvAttrStringGet(this->Handle, "PartVersion",PartVer, 20, NULL);
    unsigned long PartNum;
    PvAttrUint32Get(this->Handle, "PartNumber", &PartNum);

    if ((std::strcmp(PartVer, "B") == 0) && PartNum == 20030)
        return true;
    return false;
}

void Camera::copyCamera(Camera &cam)
{
    std::strncpy(this->CameraName, cam.CameraName, 32);
    this->Handle = cam.Handle;
    for (int i = 0; i < FRAMESCOUNT; i++)
        this->Frames[i] = cam.Frames[i];
    this->FrameSize = cam.FrameSize;
    this->Mono16 = cam.Mono16;
    this->Disconnected = cam.Disconnected;
}

void Camera::changeBinning(int scale)
{
    PvAttrUint32Set(this->Handle, "BinningX", scale);
    PvAttrUint32Set(this->Handle, "BinningY", scale);
}
