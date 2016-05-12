#-------------------------------------------------
#
# Project created by QtCreator 2016-01-25T12:04:15
#
#-------------------------------------------------
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MultiChannelViewer
TEMPLATE = app
CONFIG += static



SOURCES += main.cpp\
        multichannelviewer.cpp \
    camera.cpp \
    FFMPEGClass.cpp \
    autoexpose.cpp

HEADERS  += multichannelviewer.h \
    camera.h \
    FFMPEGClass.h \
    autoexpose.h


FORMS    += multichannelviewer.ui

DISTFILES += \
    README.md

macx: ICON = icon.icns

#OSX Libraries
macx: QMAKE_LFLAGS += -F//System/Library/Frameworks
macx: QMAKE_LFLAGS += -lstdc++.6

macx: LIBS += -framework CoreFoundation
macx: LIBS += -framework CoreVideo
macx: LIBS += -framework VideoDecodeAcceleration
macx: LIBS += -framework CoreMedia
macx: LIBS += -framework VideoToolbox
macx: LIBS += -framework Security
macx: LIBS += -lz
macx: LIBS += -lbz2.1.0
macx: LIBS += -liconv


macx: LIBS += -L/usr/local/lib -lavcodec
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavcodec.a

macx: LIBS += -L/usr/local/lib -lavdevice
macx: PRE_TARGETDEPS += /usr/local/lib/libavdevice.a

macx: LIBS += -L/usr/local/lib -lavfilter
macx: PRE_TARGETDEPS += /usr/local/lib/libavfilter.a

macx: LIBS += -L/usr/local/lib -lavutil
macx: PRE_TARGETDEPS += /usr/local/lib/libavutil.a

macx: LIBS += -L/usr/local/lib -lswscale
macx: PRE_TARGETDEPS += /usr/local/lib/libswscale.a

macx: LIBS += -L/usr/local/lib -lswresample
macx: PRE_TARGETDEPS += /usr/local/lib/libswresample.a

macx: LIBS += -L/usr/local/lib -lavformat
macx: PRE_TARGETDEPS += /usr/local/lib/libavformat.a

macx: LIBS += -L/usr/local/lib -lx264
macx: PRE_TARGETDEPS += /usr/local/lib/libx264.a

macx: LIBS += -L$$PWD/lib/x64/osx -lPvAPI
INCLUDEPATH += $$PWD/lib/x64/osx/PvAPI
DEPENDPATH += $$PWD/lib/x64/osx/PvAPI
macx: PRE_TARGETDEPS += $$PWD/lib/x64/osx/libPvAPI.a



#Windows Libraries
win32
{
    contains(QT_ARCH, i386)
    {
        INCLUDEPATH += $$PWD/lib/x86/win32/include/PvAPI
        DEPENDPATH += $$PWD/lib/x86/win32/include/PvAPI
        LIBS += -L$$PWD/lib/x86/win32/ -lPvAPI


        INCLUDEPATH += $$PWD/lib/x86/win32/include
        DEPENDPATH += $$PWD/lib/x86/win32/include
        LIBS += -L$$PWD/lib/x86/win32/ -lavcodec
        LIBS += -L$$PWD/lib/x86/win32/ -lavcodec
        LIBS += -L$$PWD/lib/x86/win32/ -lavformat
        LIBS += -L$$PWD/lib/x86/win32/ -lavutil
        LIBS += -L$$PWD/lib/x86/win32/ -lswresample
        LIBS += -L$$PWD/lib/x86/win32/ -lswscale
    }
}
