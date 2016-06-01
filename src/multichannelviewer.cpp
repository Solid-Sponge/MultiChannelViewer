#include "multichannelviewer.h"
#include "ui_multichannelviewer.h"
#include "qapplication.h"

MultiChannelViewer::MultiChannelViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MultiChannelViewer)
{
    ui->setupUi(this);
    //this->show();   //!< Displays main GUI


    /// Setting initial values for recording, screenshot, and False-coloring thresholds
    recording = false;
    screenshot_cam1 = false;
    screenshot_cam2 = false;
    monochrome = false;
    opacity_val = 0.1;
    thresh_calibrated = 2;
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
        if (this->Two_Cameras_Connected) //!< Executes if two cameras (WL and NIR) are detected
        {
            this->exposure_control = new AutoExpose(&Cam1, &Cam2);

            Cam1.moveToThread(&thread1); //!< Assigns Cam1 to Thread1
            Cam2.moveToThread(&thread2); //!< Assigns Cam2 to Thread2

            Cam2.SetMono16Bit();    //!< Enables 16-bit Mono format for Cam2 (NIR cam)


            /// Connects various slots and signals. Identical for the second set of connections
            ///
            /// The first connection connects the frameReady signal to the renderFrame slot.
            /// When the camera has finished capturing a frame, the function will execute
            /// displaying the frame on the Main GUI window
            ///
            /// The second connection connects the SIG_renderFrame_WL_Cam_Done signal to the capture() slot.
            /// This completes the loop so that when the Main GUI has finished displaying the frame,
            /// it tells the camera to capture another frame
            ///
            /// The third connection ties the camera object to its own thread, and by extension, tying all these
            /// connections to that same thread, so that the camera captures and displays an image all on its own
            /// thread
            connect(&Cam1, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_WL_Cam(Camera*)),Qt::AutoConnection);
            connect(this, SIGNAL(SIG_renderFrame_WL_Cam_Done()), &Cam1, SLOT(capture()),Qt::AutoConnection);
            connect(&thread1, SIGNAL(started()), &Cam1, SLOT(capture()));


            ///Identical to the above set of slots and signals, except for Cam2 and Cam2-specific functions
            connect(&Cam2, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_NIR_Cam(Camera*)),Qt::AutoConnection);
            connect(this, SIGNAL(SIG_renderFrame_NIR_Cam_Done()), &Cam2, SLOT(capture()), Qt::AutoConnection);
            connect(&thread2, SIGNAL(started()), &Cam2, SLOT(capture()));

            connect(this, SIGNAL(SIG_AutoExpose(QImage*,unsigned char*)),
                    exposure_control, SLOT(AutoExposure_Two_Cams(QImage*,unsigned char*)), Qt::DirectConnection);

            Cam1.captureSetup();    //!< Sets up Cam1 capture settings
            Cam2.captureSetup();    //!< Sets up Cam2 capture settings


            this->show();
            thread1.start();        //!< Initializes thread1 and starts Cam1 stream/display
            thread2.start();        //!< Initializes thread2 and starts Cam2 stream/display
        }
        else
        {
            this->exposure_control = new AutoExpose(&Cam1);

            Cam1.moveToThread(&thread1);
            if (Cam1.isWhiteLight()) //!< Executes if Cam is White Light only
            {

                /// Connects various slots and signals.
                ///
                /// The first connection connects the frameReady signal to the renderFrame slot.
                /// When the camera has finished capturing a frame, the renderFrame_WL_Cam function will execute
                /// displaying the frame on the Main GUI window
                ///
                /// The second connection connects the SIG_renderFrame_WL_Cam_Done signal to the capture() slot.
                /// This completes the loop so that when the Main GUI has finished displaying the frame,
                /// it tells the camera to capture another frame
                ///
                /// The third connection ties the camera object to its own thread, and by extension, tying all these
                /// connections to that same thread, so that the camera captures and displays an image all on its own
                /// thread
                connect(&Cam1, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_WL_Cam(Camera*)));
                connect(this, SIGNAL(SIG_renderFrame_WL_Cam_Done()), &Cam1, SLOT(capture()));
                connect(&thread1, SIGNAL(started()), &Cam1, SLOT(capture()));
                connect(this, SIGNAL(SIG_AutoExpose_WL(QImage*)),
                        exposure_control, SLOT(AutoExposure_WL_Cam(QImage*)), Qt::DirectConnection);

                this->Single_Cameras_is_WL = true;

                /// UI Tweaks (deletes all unneeded elements from NIR cam)
                delete ui->NIR_Camera;
                delete ui->cam_2;
                delete ui->cam_3;
                delete ui->Monochrome;
                delete ui->opacitySlider;
                delete ui->text_Opacity;
                delete ui->NIR_Thresh;
                delete ui->NIR_Thresh_label;

                /// UI Resizing
                ui->WL_camera->setGeometry(200,640,221,141);
                ui->Media->setGeometry(250,540,129,85);
                ui->AutoExposure->setGeometry(20,490,111,20);
                this->resize(640,900);
            }
            else if (Cam1.isNearInfrared()) //!< Executes if Cam is NIR only
            {

                /// Connects various slots and signals.
                ///
                /// The first connection connects the frameReady signal to the renderFrame slot.
                /// When the camera has finished capturing a frame, the renderFrame_WL_Cam function will execute
                /// displaying the frame on the Main GUI window
                ///
                /// The second connection connects the SIG_renderFrame_WL_Cam_Done signal to the capture() slot.
                /// This completes the loop so that when the Main GUI has finished displaying the frame,
                /// it tells the camera to capture another frame
                ///
                /// The third connection ties the camera object to its own thread, and by extension, tying all these
                /// connections to that same thread, so that the camera captures and displays an image all on its own
                /// thread
                connect(&Cam1, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_NIR_Cam(Camera*)));
                connect(this, SIGNAL(SIG_renderFrame_NIR_Cam_Done()), &Cam1, SLOT(capture()));
                connect(&thread1, SIGNAL(started()), &Cam1, SLOT(capture()));
                connect(this, SIGNAL(SIG_AutoExpose_NIR(unsigned char*)),
                        exposure_control, SLOT(AutoExposure_NIR_Cam(unsigned char*)), Qt::DirectConnection);

                this->Single_Cameras_is_WL = false;
                Cam1.SetMono16Bit();

                // UI Tweaks (deletes all unneeded elements from WL cam)
                delete ui->WL_camera;
                delete ui->cam_1;
                delete ui->cam_3;
                delete ui->Monochrome;
                delete ui->opacitySlider;
                delete ui->text_Opacity;

                // UI Resizing
                ui->cam_2->setGeometry(0,0,640,480);
                ui->NIR_Camera->setGeometry(200,640,221,141);
                ui->Media->setGeometry(250,540,129,85);
                ui->AutoExposure->setGeometry(20,490,111,20);
                ui->NIR_Thresh->setGeometry(250,800,124,24);
                ui->NIR_Thresh_label->setGeometry(250,826,124,16);
                this->resize(640,900);
            }
            Cam1.captureSetup();

            this->show();
            thread1.start();
        }

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
    return true;
}

bool MultiChannelViewer::ConnectToCam()
{
    tPvCameraInfoEx   info[2];
    unsigned long     numCameras;

    /// Creates messagebox that shows camera's connected
    int returnVal = QMessageBox::Retry;
    QMessageBox* WaitForCamera = new QMessageBox();
    WaitForCamera->setModal(true);
    WaitForCamera->setStandardButtons(QMessageBox::Ok | QMessageBox::Retry);
    WaitForCamera->setDefaultButton(QMessageBox::Ok);
    WaitForCamera->setWindowTitle("Waiting for Cameras");
    WaitForCamera->setText("Cameras found: ");
    QThread::sleep(3);

    while (returnVal == QMessageBox::Retry) //!< Allows user to refresh list of cameras if they forget to connect a camera before the application launches
    {
        numCameras = PvCameraListEx(info, 2, NULL, sizeof(tPvCameraInfoEx));
        QThread::msleep(500);
        QString CamerasFound;

        if (numCameras == 0)
            CamerasFound = tr("\n");
        if (numCameras == 1)
            CamerasFound = tr("%1\n").arg(info[0].CameraName);
        if (numCameras == 2)
            CamerasFound = tr("%1\n%2\n").arg(info[0].CameraName, info[1].CameraName);

        WaitForCamera->setInformativeText(CamerasFound);
        returnVal = WaitForCamera->exec();
    }
    delete WaitForCamera;

    /// If two cameras are found, opens them
    if ((numCameras >= 2) && (info[0].PermittedAccess & ePvAccessMaster) && (info[1].PermittedAccess & ePvAccessMaster))
    {
        Cam1.setID(info[0].UniqueId);
        Cam1.setCameraName(info[0].CameraName);

        Cam2.setID(info[1].UniqueId);
        Cam2.setCameraName(info[1].CameraName);

        /// Ensures that Cam1 is a WL cam and Cam2 is an NIR Cam if Two Cameras are connected
        if (Cam1.GrabHandleFromID() && Cam2.GrabHandleFromID())
        {
            if (!Cam1.isWhiteLight() && !Cam2.isNearInfrared())
            {
                Camera temp;
                temp.copyCamera(Cam1);
                Cam1.copyCamera(Cam2);
                Cam2.copyCamera(temp);
            }
            this->Two_Cameras_Connected = true;
            return true;
        }
        return false;
    }
    /// If only one camera is found, opens it
    if ((numCameras == 1) && (info[0].PermittedAccess & ePvAccessMaster))
    {
        Cam1.setID(info[0].UniqueId);
        Cam1.setCameraName(info[0].CameraName);
        if (Cam1.GrabHandleFromID())
        {
            this->Two_Cameras_Connected = false;
            return true;
        }
    }
    return false;
}

void MultiChannelViewer::renderFrame_WL_Cam(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();

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
    imgFrame = imgFrame.mirrored(true, false);  //!< Flips WL image. Needed due to dichroic
    ui->cam_1->setScaledContents(true);
    ui->cam_1->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_1->show(); //!< Displays image on Main GUI

    //Mutex1.lock();
    //QMutexLocker *Locker = new QMutexLocker(&Mutex1);
    *Cam1_Image = imgFrame.copy(); //!< Updates latest frame. Mutex in place to prevent race conditions
    //delete Locker;
    //Mutex1.unlock();

    qApp->processEvents(); //!< Process other GUI events

    if (screenshot_cam1) //!< Takes screenshot
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL.png");
        //timestamp = QString("/Screenshot/") + timestamp;

#ifdef _WIN32
        timestamp = QString("Screenshot\\") + timestamp;
        system ("md Screenshot");
#endif

#ifdef __APPLE__
        system("mkdir ../../../Screenshot");
        timestamp = QString("../../../Screenshot/") + timestamp;
#endif

#ifdef __linux__
        timestamp = QString("Screenshot/") + timestamp;
        system("mkdir Screenshot");
#endif

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam1 = false;
    }
    if (recording) //!< Starts recording
    {
        unsigned char* mirror_buffer;
        mirror_buffer = imgFrame.bits();

        for (int i = 0; i < static_cast<int>((this->exposure_WL / 1000000.0)*24 + 1); i++)
            Video1.WriteFrame(mirror_buffer);
    }

    if(this->Two_Cameras_Connected)
        renderFrame_Cam3(); //!< Renders third screen on GUI
    else
    {
        if (this->autoexpose)
            emit SIG_AutoExpose_WL(this->Cam1_Image);
    }

    delete[] buffer;
    emit SIG_renderFrame_WL_Cam_Done(); //!< Tells WL camera to capture another frame
}

void MultiChannelViewer::renderFrame_NIR_Cam(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();
    //PvAttrUint32Set(*CamHandle, "ExposureValue", this->exposure_NIR);

    unsigned short* rawPtr = static_cast<unsigned short*>(FramePtr1->ImageBuffer);

    //int Histogram_NIR[4096] = {0};
    int Histogram_NIR[65535] = {0};
    int pixel_count = 0;
    //thresh_calibrated = 18;
    for (int i = 0; i < FramePtr1->Height*FramePtr1->Width; i++)
    {
        if (rawPtr[i] > thresh_calibrated)
        {
            //unsigned short NIR = static_cast<unsigned short>(rawPtr[i]);
            Histogram_NIR[rawPtr[i]]++;
            pixel_count++;
        }
    }

    /// False coloring. Sets up 6 thresholds divided evenly between minVal and maxVal
    /// Displays a particular RGB value that corresponds to the intensity of the pixels
    int thresh1,thresh2,thresh3,thresh4,thresh5,thresh6 = 0;
    bool thresh2_found = false;
    bool thresh3_found = false;
    bool thresh4_found = false;
    bool thresh5_found = false;
    bool thresh6_found = false;

    thresh1 = thresh_calibrated;
    int sum = 0;
    //Integrate here to find thresholds. Thresholds placed at 17%, 34%, 51%, 68%, 85% of total pixels
    for (int k = thresh_calibrated+1; k < 4096; k++)
    {
        sum += Histogram_NIR[k];
        if (!thresh2_found && ((double) sum / (double) pixel_count) > 0.17)
        {
            thresh2 = k;
            thresh2_found = true;
        }
        if (!thresh3_found && ((double) sum / (double) pixel_count) > 0.34)
        {
            thresh3 = (thresh2 >= k) ? thresh2+1 : k;
            thresh3_found = true;
        }
        if (!thresh4_found && ((double) sum / (double) pixel_count) > 0.51)
        {
            thresh4 = (thresh3 >= k) ? thresh3+1 : k;
            thresh4_found = true;
        }
        if (!thresh5_found && ((double) sum / (double) pixel_count) > 0.68)
        {
            thresh5 = (thresh4 >= k) ? thresh4+1 : k;
            thresh5_found = true;
        }
        if (!thresh6_found && ((double) sum / (double) pixel_count) > 0.85)
        {
            thresh6 = (thresh5 >= k) ? thresh5+1 : k;
            thresh6_found = true;
        }
    }


    unsigned char* buffer = new unsigned char[3*FramePtr1->Height*FramePtr1->Width];
    unsigned char* bufferPtr = buffer;

    QImage imgFrame(FramePtr1->Width, FramePtr1->Height, QImage::Format_RGB888);
    for (int i = 0; i < FramePtr1->Height; i++)
    {
        for (int j = 0; j < FramePtr1->Width; j++)
        {
            unsigned short counts = *rawPtr;
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
                //b = 255*(1-(counts-thresh3)/space);
                b = 0;
            }
            else if (counts <= thresh5)
            {
                //r = 255*(counts-thresh4)/space;
                r = 255;
                g = 255;
                b = 0;
            }
            else if (counts <= thresh6)
            {
                r = 255;
                g = 0;
                //g = 255*(1-(counts-thresh5)/space);
                b = 0;
            }
            /*else if (counts <= thresh7)
            {
                r = 255;
                g = 255*(counts-thresh6)/space;
                b = 255*(counts-thresh6)/space;
            }*/
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

            /*imgFrame.setPixel(2*j, 2*i, color);
            imgFrame.setPixel(2*j + 1, 2*i, color);
            imgFrame.setPixel(2*j, 2*i + 1, color);
            imgFrame.setPixel(2*j + 1, 2*i + 1, color);*/
            rawPtr++;
        }
    }
    ui->cam_2->setScaledContents(true);
    ui->cam_2->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_2->show();

    *Cam2_Image = imgFrame;
    std::memcpy(Cam2_Image_Raw, FramePtr1->ImageBuffer, FramePtr1->ImageBufferSize);

    if (!this->Two_Cameras_Connected && this->autoexpose)
        emit SIG_AutoExpose_NIR(this->Cam2_Image_Raw);


    qApp->processEvents();

    if (screenshot_cam2)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_NIR.png");

#ifdef _WIN32
        timestamp = QString("Screenshot\\") + timestamp;
        system ("md Screenshot");
#endif

#ifdef __APPLE__
        system("mkdir ../../../Screenshot");
        timestamp = QString("../../../Screenshot/") + timestamp;
#endif

#ifdef __linux__
        timestamp = QString("Screenshot/") + timestamp;
        system("mkdir Screenshot");
#endif

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam2 = false;
    }

    if (recording)
    {
        for (int i = 0; i < static_cast<int>((this->exposure_NIR / 1000000.0)*24 + 1); i++)
                Video2.WriteFrame(buffer);
    }

    //renderFrame_Cam3(); //!< Doesn't need to be called in both render functions.


    delete[] buffer;

    emit SIG_renderFrame_NIR_Cam_Done();
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

           data[0] = val; //!< Pointer Arithmetic. Image is 3-byte aligned, each representing R, G, or B.
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
        //QtConcurrent::run(this, &MultiChannelViewer::AutoExposure);
        emit SIG_AutoExpose(this->Cam1_Image, this->Cam2_Image_Raw);

    if (screenshot_cam3)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL+NIR.png");

#ifdef _WIN32
        timestamp = QString("Screenshot\\") + timestamp;
        system ("md Screenshot");
#endif

#ifdef __APPLE__
        timestamp = QString("../../../Screenshot/") + timestamp;
        system("mkdir ../../../Screenshot");
#endif

#ifdef __linux__
        timestamp = QString("Screenshot/") + timestamp;
        system("mkdir Screenshot");
#endif

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam3 = false;
    }

    if (recording)  //!< Use foreground * alpha + background * (1-alpha)
    {
        QImage RGB24(WIDTH, HEIGHT, QImage::Format_RGB888);
        unsigned char* Cam1_Image_ptr = (monochrome) ? Cam1_Image_Mono.bits() : Cam1_Image->bits();
        unsigned char* Cam2_Image_ptr = Cam2_Image->bits();
        unsigned char* RGB24_ptr = RGB24.bits();

        for (int i = 0; i < WIDTH*HEIGHT; i++)
        {
            RGB24_ptr[0] = Cam1_Image_ptr[0] * (1.0
                                                -opacity_val) + Cam2_Image_ptr[0] * (opacity_val);
            RGB24_ptr[1] = Cam1_Image_ptr[1] * (1.0-opacity_val) + Cam2_Image_ptr[1] * (opacity_val);
            RGB24_ptr[2] = Cam1_Image_ptr[2] * (1.0-opacity_val) + Cam2_Image_ptr[2] * (opacity_val);
            RGB24_ptr += 3;
            Cam1_Image_ptr += 3;
            Cam2_Image_ptr += 3;
        }

        //RGB24 = RGB24.convertToFormat(QImage::Format_RGB888);
        int exposure = (this->exposure_WL < this->exposure_NIR) ? this->exposure_WL : this->exposure_NIR;
        for (int i = 0; i < static_cast<int>((exposure / 1000000.0)*24 + 1); i++)
            Video3.WriteFrame(RGB24.bits());
    }
}

void MultiChannelViewer::calibrate_NIR_thresh(QAbstractButton *button)
{
    QMessageBox::StandardButton btn = Calibrate_Window->standardButton(button);
    if (btn == QMessageBox::Ok)
    {
        unsigned short* Image_NIR_data = new unsigned short[HEIGHT*WIDTH];
        std::memcpy(Image_NIR_data, Cam2_Image_Raw, HEIGHT*WIDTH*2);
        unsigned long long sum = 0;

        //for (int i = 0; i < HEIGHT*WIDTH; i++)
        for (int i = 1; i < HEIGHT; i++)
        {
            for (int j = 1; j < WIDTH; j++)
            {
                sum += Image_NIR_data[i*WIDTH + j];
            }
            //sum += Image_NIR_data[i];
        }
        int average = sum / ((HEIGHT-1)*(WIDTH-1));
        this->thresh_calibrated = average + 2;
        ui->NIR_Thresh->setValue(thresh_calibrated);
        delete Image_NIR_data;
    }
}

/////////////////////////////////////////////////////////////
///////// GUI related stuff below (event handlers) //////////
/////////////////////////////////////////////////////////////

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
    Cam1.captureEnd();

    if (this->Two_Cameras_Connected)
    {
        thread2.quit();
        Cam2.captureEnd();
    }
        thread2.quit();

    delete[] Cam2_Image_Raw;

    PvUnInitialize();
    QApplication::exit(0);
}


void MultiChannelViewer::on_Record_toggled(bool checked)
{
    if (checked)
    {
        QString timestamp_filename_WL;
        QString timestamp_filename_NIR;
        QString timestamp_filename_WL_NIR;

        #ifdef _WIN32
        system("md Video");
        #endif

        #ifdef __APPLE__
        system("mkdir ../../../Video");
        #endif

        if (this->Two_Cameras_Connected || this->Single_Cameras_is_WL)
        {
            timestamp_filename_WL = QDateTime::currentDateTime().toString();
            timestamp_filename_WL.append("_WL.avi");
            timestamp_filename_WL.replace(QString(" "), QString("_"));
            timestamp_filename_WL.replace(QString(":"), QString("-"));

            #ifdef _WIN32
            timestamp_filename_WL = QString("Video\\") + timestamp_filename_WL;
            #endif

            #ifdef __APPLE__
            timestamp_filename_WL = QString("../../../Video/") + timestamp_filename_WL;
            #endif

            char* filename_WL = (char*) timestamp_filename_WL.toStdString().c_str();
            this->Video1.SetupVideo(filename_WL, 640, 480, 12, 2, 750000); //bitrate = 40000000
        }

        if (this->Two_Cameras_Connected || !this->Single_Cameras_is_WL)
        {
            timestamp_filename_NIR = QDateTime::currentDateTime().toString();
            timestamp_filename_NIR.append("_NIR.avi");
            timestamp_filename_NIR.replace(QString(" "), QString("_"));
            timestamp_filename_NIR.replace(QString(":"), QString("-"));

            #ifdef _WIN32
            timestamp_filename_NIR = QString("Video\\") + timestamp_filename_NIR;
            #endif

            #ifdef __APPLE__
            timestamp_filename_NIR = QString("../../../Video/") + timestamp_filename_NIR;
            #endif

            char* filename_NIR = (char*) timestamp_filename_NIR.toStdString().c_str();
            this->Video2.SetupVideo(filename_NIR, 640, 480, 12, 2, 750000);
        }

        if (this->Two_Cameras_Connected)
        {
            timestamp_filename_WL_NIR = QDateTime::currentDateTime().toString();
            timestamp_filename_WL_NIR.append("_WL+NIR.avi");
            timestamp_filename_WL_NIR.replace(QString(" "), QString("_"));
            timestamp_filename_WL_NIR.replace(QString(":"), QString("-"));

            #ifdef _WIN32
            timestamp_filename_WL_NIR = QString("Video\\") + timestamp_filename_WL_NIR;
            #endif

            #ifdef __APPLE__
            timestamp_filename_WL_NIR = QString("../../../Video/") + timestamp_filename_WL_NIR;
            #endif

            char* filename_WL_NIR = (char*) timestamp_filename_WL_NIR.toStdString().c_str();
            this->Video3.SetupVideo(filename_WL_NIR, 640, 480, 12, 2, 750000);
        }

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
    if (this->Two_Cameras_Connected)
        PvAttrUint32Set(*(Cam2.getHandle()), "RegionX", arg1);
    else
        PvAttrUint32Set(*(Cam1.getHandle()), "RegionX", arg1);
}

void MultiChannelViewer::on_RegionY_NIR_valueChanged(int arg1)
{
    if (this->Two_Cameras_Connected)
        PvAttrUint32Set(*(Cam2.getHandle()), "RegionY", arg1);
    else
        PvAttrUint32Set(*(Cam1.getHandle()), "RegionY", arg1);
}

void MultiChannelViewer::on_AutoExposure_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked)
    {
        this->autoexpose = true;
        if (this->Two_Cameras_Connected)
        {
            ui->WL_Exposure->setReadOnly(true);
            ui->NIR_Exposure->setReadOnly(true);
        }
        else
        {
            if (this->Single_Cameras_is_WL)
                ui->WL_Exposure->setReadOnly(true);
            if (!this->Single_Cameras_is_WL)
                ui->NIR_Exposure->setReadOnly(true);
        }
    }
    if (arg1 == Qt::Unchecked)
    {
        this->autoexpose = false;
        if (this->Two_Cameras_Connected)
        {
            ui->WL_Exposure->setReadOnly(false);
            ui->NIR_Exposure->setReadOnly(false);
        }
        else
        {
            if (this->Single_Cameras_is_WL)
                ui->WL_Exposure->setReadOnly(false);
            if (!this->Single_Cameras_is_WL)
                ui->NIR_Exposure->setReadOnly(false);
        }
    }
}

void MultiChannelViewer::on_WL_Exposure_valueChanged(int arg1)
{
    this->exposure_control->ChangeExposure_WL(static_cast<unsigned int>(arg1));
    tPvHandle *cam = Cam1.getHandle();
    if (cam != NULL)
        PvAttrUint32Set(*cam, "ExposureValue", arg1);
}

void MultiChannelViewer::on_NIR_Exposure_valueChanged(int arg1)
{
    this->exposure_control->ChangeExposure_NIR(static_cast<unsigned int>(arg1));
    tPvHandle *cam;
    cam = (this->Two_Cameras_Connected) ? this->Cam2.getHandle() : this->Cam1.getHandle();
    if (cam != NULL)
        PvAttrUint32Set(*cam, "ExposureValue", arg1);
}

void MultiChannelViewer::on_actionCalibrate_NIR_triggered()
{
    if (Single_Cameras_is_WL && !Two_Cameras_Connected)
    {
        QMessageBox* InvalidMsg = new QMessageBox();
        InvalidMsg->setIcon(QMessageBox::Critical);
        InvalidMsg->setModal(true);
        InvalidMsg->setText("ERROR: No NIR Camera detected.");
        InvalidMsg->setAttribute(Qt::WA_DeleteOnClose);
        InvalidMsg->show();
        return;
    }
    Calibrate_Window = new QMessageBox();
    Calibrate_Window->setModal(true);
    Calibrate_Window->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    Calibrate_Window->setDefaultButton(QMessageBox::Ok);
    Calibrate_Window->setWindowTitle(tr("NIR Calibration"));
    Calibrate_Window->setText("This will calibrate the NIR thresholds.\nPoint the NIR camera at an empty space.");
    Calibrate_Window->setAttribute(Qt::WA_DeleteOnClose);
    Calibrate_Window->open(this, SLOT(calibrate_NIR_thresh(QAbstractButton*)));
}

void MultiChannelViewer::on_actionAbout_triggered()
{
    QString title = "MultiChannelViewer Copyright (C) 2016 Michael Rossi";
    QString message =  "MultiChannelViewer Copyright (C) 2016 Michael Rossi\n\n\
This program is free software: you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation, either version 3 of the License, or \
(at your option) any later version.\n\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
GNU General Public License for more details.\n\n\
You should have received a copy of the GNU General Public License \
along with this program.  If not, see <http://www.gnu.org/licenses/>.";

    QMessageBox::about(this, title, message);
}

void MultiChannelViewer::on_NIR_Thresh_valueChanged(int arg1)
{
    this->thresh_calibrated = arg1;
}
