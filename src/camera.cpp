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
        //delete[] static_cast<unsigned char*>(Frames[0].ImageBuffer);
   PvCommandRun(this->Handle, "AcquisitionStop");

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
        PvAttrUint32Set(this->Handle, "RegionX", 295);  //x = 295
        PvAttrUint32Set(this->Handle, "RegionY", 236);  //y = 236
    }
    else
    {
        PvAttrUint32Set(this->Handle, "StreamBytesPerSecond", 115000000 / 2 + 20000000);
        PvAttrEnumSet(this->Handle, "PixelFormat", "Bayer8");

        PvAttrUint32Set(this->Handle, "BinningX", 1);
        PvAttrUint32Set(this->Handle, "BinningY", 1);

        PvAttrUint32Set(this->Handle, "Width", 640); //x = 269
        PvAttrUint32Set(this->Handle, "Height", 480); //y = 332
        PvAttrUint32Set(this->Handle, "RegionX", 523);
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

    tPvErr errcode;
    do
    {
        errcode = PvCaptureQueueFrame(this->Handle, &(this->Frames[0]), NULL);
        PvCaptureWaitForFrameDone(this->Handle, &(this->Frames[0]), PVINFINITE);
    }while (errcode != ePvErrSuccess && errcode != ePvErrUnplugged);

    //if (errcode != ePvErrSuccess)
        //std::cout << "Frame is Kill" << std::endl;

    PvCommandRun(this->Handle, "AcquisitionStop");
    if (errcode == ePvErrUnplugged)
    {
        this->Disconnected = true;
        QMessageBox errBox;
        errBox.critical(0,"Error","Camera unplugged. Please replug camera and restart program.");
        errBox.setFixedSize(500,200);
        QApplication::exit(1);
    }


    if (Mono16)
    {
        medianFilter(3);
        /*// Median Filter
        unsigned short* rawPtr = static_cast<unsigned short*>(Frames[0].ImageBuffer);
        unsigned char* filter = new unsigned char[Frames[0].ImageSize];
        memset(filter, 0, Frames[0].ImageSize);
        unsigned short* filterPtr = reinterpret_cast<unsigned short*>(filter);
        QVector<unsigned short> window(9);    
        for (int i = 1; i < Frames[0].Height - 1; i++)
        {
            for (int j = 1; j < Frames[0].Width - 1; j++)
            {
                window[0] = rawPtr[Frames[0].Width*(i-1) + (j-1)];
                window[1] = rawPtr[Frames[0].Width*(i-1) + (j)];
                window[2] = rawPtr[Frames[0].Width*(i-1) + (j+1)];
                window[3] = rawPtr[Frames[0].Width*(i) + (j-1)];
                window[4] = rawPtr[Frames[0].Width*(i) + (j)];
                window[5] = rawPtr[Frames[0].Width*(i) + (j+1)];
                window[6] = rawPtr[Frames[0].Width*(i+1) + (j-1)];
                window[7] = rawPtr[Frames[0].Width*(i+1) + (j)];
                window[8] = rawPtr[Frames[0].Width*(i+1) + (j+1)];

                qSort(window);
                filterPtr[Frames[0].Width*(i) + j] = window[4];
            }
        }*/




        /*//Binning Correcting Factor
        unsigned char * correct = new unsigned char[Frames[0].ImageSize];
        unsigned short* correctPtr = reinterpret_cast<unsigned short*>(correct);
        for (int i = 0; i < Frames[0].Height/2; i++)
        {
            for (int j = 0; j < Frames[0].Width/2; j++)
            {
                //correctPtr[Frames[0].Width*2*i + 2*j] = rawPtr[Frames[0].Width*i + j];
                correctPtr[coord(2*i,2*j,Frames[0].Width)] = filterPtr[Frames[0].Width*(i+100) + (j+135)];

                //correctPtr[Frames[0].Width*2*i + 2*j+1] = rawPtr[Frames[0].Width*i + j];
                correctPtr[coord(2*i,(2*j + 1), Frames[0].Width)] = filterPtr[Frames[0].Width*(i+100) + (j+135)];

                //correctPtr[Frames[0].Width*2*i + 1 + 2*j] = rawPtr[Frames[0].Width*i + j];
                correctPtr[coord((2*i + 1) , 2*j, Frames[0].Width)] = filterPtr[Frames[0].Width*(i+100) + (j+135)];

                //correctPtr[Frames[0].Width*2*i + 1 + 2*j + 1] = rawPtr[Frames[0].Width*i + j];
                correctPtr[coord((2*i + 1), (2*j + 1), Frames[0].Width)] = filterPtr[Frames[0].Width*(i+100) + (j+135)];
            }
        }*/
        //memcpy(rawPtr,correctPtr,Frames[0].ImageSize);
        //memcpy(rawPtr, filterPtr, Frames[0].ImageSize);

        //delete[] filter;
        //delete[] correct;
    }

    emit frameReady(this);
}

void Camera::SetMono16Bit()
{
    this->Mono16 = true;
}

bool Camera::isWhiteLight()
{
    char PartVer[20];
    tPvErr uwut = PvAttrStringGet(this->Handle, "PartVersion",PartVer, 20, NULL);
    unsigned long PartNum;
    tPvErr uwut2 = PvAttrUint32Get(this->Handle, "PartNumber", &PartNum);

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

void Camera::medianFilter(int radius)
{
    unsigned short* rawPtr = static_cast<unsigned short*>(Frames[0].ImageBuffer);
    unsigned char* filter = new unsigned char[Frames[0].ImageSize];
    memset(filter, 0, Frames[0].ImageSize);
    unsigned short* filterPtr = reinterpret_cast<unsigned short*>(filter);

    int h = Frames[0].Height;
    int w = Frames[0].Width;
    int Histogram[4096] = {0};
    int median = 0;
    int middle_element = (((2*radius + 1)*(2*radius+1))/2);

    for (int j = 0; j < (2*radius + 1); j++)
    {
        for (int i = 0; i < (2*radius + 1); i++)
        {
            Histogram[rawPtr[coord(i,j,w)]]++;
        }
    }

    int sum = 0;
    int k = 0;
    while (sum <= middle_element)
    {
        sum += Histogram[k];
        k++;
    }
    k--;
    median = k;
    filterPtr[coord(radius,radius,w)] = median;

    int x = radius + 1;
    int y = radius;
    while (true)
    {
        for (x = radius + 1; x < (w - radius); x++) //Replace then Slide right
        {
            for (int i = 0; i < (2*radius + 1); i++)
            {
                Histogram[rawPtr[coord(x - (radius+1), y - radius + i, w)]]--;
                Histogram[rawPtr[coord(x + radius, y - radius + i, w)]]++;
            }

            sum = 0;
            k = 0;
            while (sum <= middle_element)
            {
                sum += Histogram[k];
                k++;
            }
            median = k;
            filterPtr[coord(x,y,w)] = median;
        }
        x--;
        y++;

        //if (y out of bounds) break;
        if (y + radius >= h)
            break;

        for (int i = 0; i < (2*radius + 1); i++) //Slide down
        {
            Histogram[rawPtr[coord(x - radius + i, y-(radius+1),w)]]--;
            Histogram[rawPtr[coord(x - radius + i, y + radius, w)]]++;
        }

        sum = 0;
        k = 0;
        while (sum <= middle_element)
        {
            sum += Histogram[k];
            k++;
        }
        median = k;
        filterPtr[coord(x,y,w)] = median;
        x--; //Initial Slide left

        while (x >= radius) //Replace then Slide left
        {

            for (int i = 0; i < (2*radius + 1); i++)
            {
                Histogram[rawPtr[coord(x + (radius+1), y - radius + i, w)]]--;
                Histogram[rawPtr[coord(x - radius, y - radius + i, w)]]++;
            }

            sum = 0;
            k = 0;
            while (sum <= middle_element)
            {
                sum += Histogram[k];
                k++;
            }
            median = k;
            filterPtr[coord(x,y,w)] = median;
            x--;
        }
        x++;
        y++; //Initial Slide down

        if (y + radius >= h)
            break;

        for (int i = 0; i < (2*radius + 1); i++) //Slide down
        {
            Histogram[rawPtr[coord(x - radius + i, y-(radius+1),w)]]--;
            Histogram[rawPtr[coord(x - radius + i, y + radius, w)]]++;
        }

        sum = 0;
        k = 0;
        while (sum <= middle_element)
        {
            sum += Histogram[k];
            k++;
        }
        median = k;
        filterPtr[coord(x,y,w)] = median;

    }
    memcpy(rawPtr, filterPtr, Frames[0].ImageSize);
    delete[] filter;
}
