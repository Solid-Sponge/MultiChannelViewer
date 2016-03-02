#include "multichannelviewer.h"
#include "ui_multichannelviewer.h"
#include "qapplication.h"

//#define minVal 0
//#define maxVal 4000


MultiChannelViewer::MultiChannelViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MultiChannelViewer)
{
    ui->setupUi(this);
    this->show();   //!< Displays main GUI


    /// Setting initial values for recording, screenshot, and False-coloring thresholds
    minVal = 0;
    maxVal = 4000;
    recording = false;
    screenshot_cam1 = false;
    screenshot_cam2 = false;
    monochrome = false;
    opacity_val = 0.1;
    ui->minVal->setRange(0,4095);
    ui->maxVal->setRange(0,4095);
    ui->minVal->setValue(10);
    ui->maxVal->setValue(200);

    Cam1_Image = new QImage(WIDTH, HEIGHT, QImage::Format_RGB888);
    Cam2_Image = new QImage(WIDTH, HEIGHT, QImage::Format_RGB888);

    Cam1_Image->fill(0);
    Cam2_Image->fill(0);

    if (this->InitializePv() && this->ConnectToCam()) //!< Executes if PvAPI initializes and Cameras connect successfully
    {
        Cam1.moveToThread(&thread1); //!< Assigns Cam1 to Thread1
        Cam2.moveToThread(&thread2); //!< Assigns Cam2 to Thread2

        Cam2.SetMono16Bit();    //!< Enables 16-bit Mono format for Cam2 (NIR cam)


        /// Connects various slots and signals. Identical for the second set of connections
        ///
        /// The first connection connects the frameReady signal to the renderFrame slot.
        /// When the camera has finished capturing a frame, the renderFrame_Cam1 function will execute
        /// displaying the frame on the Main GUI window
        ///
        /// The second connection connects the renderFrame_Cam1_Done signal to the capture() slot.
        /// This completes the loop so that when the Main GUI has finished displaying the frame,
        /// it tells the camera to capture another frame
        ///
        /// The third connection ties the camera object to its own thread, and by extension, tying all these
        /// connections to that same thread, so that the camera captures and displays an image all on its own
        /// thread
        connect(&Cam1, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_Cam1(Camera*)),Qt::AutoConnection);
        connect(this, SIGNAL(renderFrame_Cam1_Done()), &Cam1, SLOT(capture()),Qt::AutoConnection);
        connect(&thread1, SIGNAL(started()), &Cam1, SLOT(capture()));


        ///Identical to the above set of slots and signals, except for Cam2 and Cam2-specific functions
        connect(&Cam2, SIGNAL(frameReady(Camera*)), this, SLOT(renderFrame_Cam2(Camera*)),Qt::AutoConnection);
        connect(this, SIGNAL(renderFrame_Cam2_Done()), &Cam2, SLOT(capture()), Qt::AutoConnection);
        connect(&thread2, SIGNAL(started()), &Cam2, SLOT(capture()));

        Cam1.captureSetup();    //!< Sets up Cam1 capture settings
        Cam2.captureSetup();    //!< Sets up Cam2 capture settings

        thread1.start();        //!< Initializes thread1 and starts Cam1 stream/display
        thread2.start();        //!< Initializes thread2 and starts Cam2 stream/display

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

    QThread::sleep(3);
    if (PvCameraCount() < 2)
    {
        QMessageBox errBox;
        errBox.critical(0,"Error","2 Cameras not found. Please plug in 2 cameras then press Ok to continue.");
        errBox.setFixedSize(500,200);
    }
    while (PvCameraCount() < 2)
    {
        QThread::msleep(100);
        qApp->processEvents();
    }

    std::cout << "Cameras Found: " << PvCameraCount() << std::endl;
    return true;
}

bool MultiChannelViewer::ConnectToCam()
{
    tPvCameraInfoEx   info[2];
    unsigned long     numCameras;
    numCameras = PvCameraListEx(info, 2, NULL, sizeof(tPvCameraInfoEx));
    // Open the first two cameras found, if it’s not already open. (See
    // function reference for PvCameraOpen for a more complex example.)
    if ((numCameras >= 1) && (info[0].PermittedAccess & ePvAccessMaster) && (info[1].PermittedAccess & ePvAccessMaster))
    {
        Cam1.setID(info[0].UniqueId);
        Cam1.setCameraName(info[0].CameraName);

        Cam2.setID(info[1].UniqueId);
        Cam2.setCameraName(info[1].CameraName);

        if (Cam1.GrabHandleFromID() && Cam2.GrabHandleFromID())
        {
            if (!Cam1.isWhiteLight() && !Cam2.isNearInfrared())
            {
                Camera temp;
                temp.copyCamera(Cam1);
                Cam1.copyCamera(Cam2);
                Cam2.copyCamera(temp);
            }

            return true;
        }
        return false;
    }
    return false;
}


void MultiChannelViewer::renderFrame_Cam1(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();
    //PvAttrUint32Set(*CamHandle, "ExposureValue", 15000);

    unsigned long line_padding = ULONG_PADDING(FramePtr1->Width*3);
    unsigned long line_size = (FramePtr1->Width*3) + line_padding;
    unsigned long buffer_size = line_size * FramePtr1->Height;
    unsigned char* buffer = new unsigned char[buffer_size];
    unsigned char* bufferPtr = buffer;

    PvUtilityColorInterpolate(FramePtr1, &buffer[0], &buffer[1], &buffer[2], 2, 0);

    QImage imgFrame(FramePtr1->Width, FramePtr1->Height, QImage::Format_RGB888);

    /// Displays frame on main GUI
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
            //imgFrame.setPixel(FramePtr1->Width - j - 1, i, color); //!< Displays pixel with (r,g,b) at location j,i
            imgFrame.setPixel(j, i, color);
        }
    }
    imgFrame = imgFrame.mirrored(true, false);
    ui->cam_1->setScaledContents(true);
    ui->cam_1->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_1->show();

    *Cam1_Image = imgFrame;
    qApp->processEvents();

    if (screenshot_cam1)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL.png");
        //timestamp = QString("/Screenshot/") + timestamp;

        QFile file(timestamp);
        //QFile file(QCoreApplication::applicationDirPath() + "/Screenshot/" + timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam1 = false;
    }
    if (recording)
    {
        //unsigned char* mirror_buffer = new unsigned char[buffer_size];
        //unsigned char* mirror_buffer_Ptr = mirror_buffer;
        unsigned char* mirror_buffer;
        mirror_buffer = imgFrame.bits();
        //Video1.WriteFrame(buffer);
        Video1.WriteFrame(mirror_buffer);
    }

    renderFrame_Cam3();

    delete[] buffer;
    emit renderFrame_Cam1_Done();
}

void MultiChannelViewer::renderFrame_Cam2(Camera* cam)
{
    tPvFrame* FramePtr1 = cam->getFramePtr();
    tPvHandle* CamHandle = cam->getHandle();
    PvAttrUint32Set(*CamHandle, "ExposureValue", 500000);


    //unsigned short* rawPtr = (unsigned short)FramePtr1->ImageBuffer;
    unsigned short* rawPtr = static_cast<unsigned short*>(FramePtr1->ImageBuffer);
    //unsigned char* rawPtr = (unsigned char*)FramePtr1->ImageBuffer;

    /// False coloring. Sets up 7 thresholds divided evenly between minVal and maxVal
    /// Displays a particular RGB value that corresponds to the intensity of the pixels
    int thresh1,thresh2,thresh3,thresh4,thresh5,thresh6,thresh7;
    thresh1 = minVal;
    thresh7 = maxVal;
    int space = (thresh7 - thresh1)/6;
    thresh2 = thresh1 + space;
    thresh3 = thresh2 + space;
    thresh4 = thresh3 + space;
    thresh5 = thresh4 + space;
    thresh6 = thresh5 + space;


    unsigned char* buffer = new unsigned char[3*FramePtr1->Height*FramePtr1->Width];
    unsigned char* bufferPtr = buffer;

    QImage imgFrame(FramePtr1->Width, FramePtr1->Height, QImage::Format_RGB888);
    for (int i = 0; i < FramePtr1->Height; i++)
    {
        for (int j = 0; j < FramePtr1->Width; j++)
        {
            unsigned short counts = *rawPtr;
            //unsigned char counts = *rawPtr;
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
                b = 255*(1-(counts-thresh3)/space);
            }
            else if (counts <= thresh5)
            {
                r = 255*(counts-thresh4)/space;
                g = 255;
                b = 0;
            }
            else if (counts <= thresh6)
            {
                r = 255;
                g = 255*(1-(counts-thresh5)/space);
                b = 0;
            }
            else if (counts <= thresh7)
            {
                r = 255;
                g = 255*(counts-thresh6)/space;
                b = 255*(counts-thresh6)/space;
            }
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
            rawPtr++;
        }
    }
    ui->cam_2->setScaledContents(true);
    ui->cam_2->setPixmap(QPixmap::fromImage(imgFrame));
    ui->cam_2->show();

    *Cam2_Image = imgFrame;
    qApp->processEvents();

    if (screenshot_cam2)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_NIR.png");

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam2 = false;
    }

    if (recording)
    {
        Video2.WriteFrame(buffer);
    }

    renderFrame_Cam3();

    delete[] buffer;

    emit renderFrame_Cam2_Done();
}

void MultiChannelViewer::renderFrame_Cam3()
{
    if (monochrome)
    {
        *Cam1_Image = Cam1_Image->convertToFormat(QImage::Format_RGB32);

        unsigned int *data = (unsigned int*)Cam1_Image->bits();
        int pixelCount = Cam1_Image->width() * Cam1_Image->height();

        // Convert each pixel to grayscale
        for(int i = 0; i < pixelCount; ++i)
        {
           int val = qGray(*data);
           *data = qRgba(val, val, val, qAlpha(*data));
           //*data = qRgb(val, val, val);
           ++data;
        }
    }

    QPixmap imgFrame(Cam1_Image->size());
    QPainter p(&imgFrame);

    p.drawImage(QPoint(0,0), *Cam1_Image);
    p.setOpacity(opacity_val);
    p.drawImage(QPoint(0,0), *Cam2_Image);
    p.end();

    ui->cam_3->setScaledContents(true);
    ui->cam_3->setPixmap(imgFrame);
    ui->cam_3->show();
    qApp->processEvents();

    if (screenshot_cam3)
    {
        QString timestamp = QDateTime::currentDateTime().toString();
        timestamp.replace(QString(" "), QString("_"));
        timestamp.replace(QString(":"), QString("-"));
        timestamp.append("_WL+NIR.png");

        QFile file(timestamp);
        file.open(QIODevice::WriteOnly);
        imgFrame.save(&file, "PNG");
        file.close();
        screenshot_cam3 = false;
    }

    if (recording)
    {
        QImage RGB24 = imgFrame.toImage();
        RGB24 = RGB24.convertToFormat(QImage::Format_RGB888);
        Video3.WriteFrame(RGB24.bits());
    }
}

void MultiChannelViewer::closeEvent(QCloseEvent *event)
{
    if (recording)
    {
        Video1.CloseVideo();
        Video2.CloseVideo();
        Video3.CloseVideo();
    }

    thread1.quit();
    thread2.quit();

    Cam1.captureEnd();
    Cam2.captureEnd();

    PvUnInitialize();
    QApplication::exit(0);
}


void MultiChannelViewer::on_minVal_valueChanged(int value)
{
    if (value < this->maxVal - 150)
    {
        this->minVal = value;
        ui->minVal_spinbox->setValue(value);

    }
    else
    {
        this->minVal = this->maxVal - 150;
        ui->minVal_spinbox->setValue(this->maxVal - 150);
    }
}

void MultiChannelViewer::on_maxVal_valueChanged(int value)
{
    if (value > this->minVal + 150)
    {
        this->maxVal = value;
        ui->maxVal_spinbox->setValue(value);
    }
    else
    {
        this->maxVal = this->minVal + 150;
        ui->maxVal_spinbox->setValue(this->minVal + 150);
    }
}

void MultiChannelViewer::on_minVal_spinbox_valueChanged(int arg1)
{
    if (arg1 < this->maxVal - 150)
    {
        this->minVal = arg1;
        ui->minVal_spinbox->setValue(arg1);
        ui->minVal->setValue(arg1);
    }
    else
    {
        this->minVal = this->maxVal - 150;
        ui->minVal_spinbox->setValue(this->maxVal - 150);
        ui->minVal->setValue(this->maxVal - 150);
    }
}

void MultiChannelViewer::on_maxVal_spinbox_valueChanged(int arg1)
{
    if (arg1 > this->minVal + 150)
    {
        this->maxVal = arg1;
        ui->maxVal_spinbox->setValue(arg1);
        ui->maxVal->setValue(arg1);
    }
    else
    {
        this->maxVal = this->minVal + 150;
        ui->maxVal_spinbox->setValue(this->minVal + 150);
        ui->maxVal->setValue(this->minVal + 150);
    }
}

void MultiChannelViewer::on_minVal_sliderMoved(int position)
{
    if (position > this->maxVal - 150)
    {
        ui->minVal_spinbox->setValue(position);
    }
}

void MultiChannelViewer::on_maxVal_sliderMoved(int position)
{
    if (position > this->minVal + 150)
    {
        ui->maxVal_spinbox->setValue(position);
    }
}

void MultiChannelViewer::on_Record_toggled(bool checked)
{
    if (checked)
    {
        QString timestamp_filename_WL = QDateTime::currentDateTime().toString();
        timestamp_filename_WL.append("_WL.avi");
        timestamp_filename_WL.replace(QString(" "), QString("_"));
        timestamp_filename_WL.replace(QString(":"), QString("-"));
        timestamp_filename_WL = QString("Video/") + timestamp_filename_WL;

        QString timestamp_filename_NIR = QDateTime::currentDateTime().toString();
        timestamp_filename_NIR.append("_NIR.avi");
        timestamp_filename_NIR.replace(QString(" "), QString("_"));
        timestamp_filename_NIR.replace(QString(":"), QString("-"));

        QString timestamp_filename_WL_NIR = QDateTime::currentDateTime().toString();
        timestamp_filename_WL_NIR.append("_WL+NIR.avi");
        timestamp_filename_WL_NIR.replace(QString(" "), QString("_"));
        timestamp_filename_WL_NIR.replace(QString(":"), QString("-"));

        char* filename_WL = (char*) timestamp_filename_WL.toStdString().c_str();
        this->Video1.SetupVideo(filename_WL, 640, 480, 15, 2, 750000); //bitrate = 40000000

        char* filename_NIR = (char*) timestamp_filename_NIR.toStdString().c_str();
        this->Video2.SetupVideo(filename_NIR, 640, 480, 15, 2, 750000);

        char* filename_WL_NIR = (char*) timestamp_filename_WL_NIR.toStdString().c_str();
        this->Video3.SetupVideo(filename_WL_NIR, 640, 480, 15, 2, 750000);

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

void MultiChannelViewer::on_checkBox_stateChanged(int arg1)
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
