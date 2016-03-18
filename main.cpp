//#define _OSX
//#define _x64
//#define FRAMESCOUNT 3

#include "multichannelviewer.h"
#include <QApplication>
#include <QThread>
#include <QMessageBox>
#include <QLabel>
#include <QFile>
#include <QImage>

#include <iostream>
#include <PvApi.h>
#include <PvRegIo.h>
#include <camera.h>

#include <FFMPEGClass.h>

#include <ctime>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MultiChannelViewer w;

    w.show();


    return a.exec();
}
