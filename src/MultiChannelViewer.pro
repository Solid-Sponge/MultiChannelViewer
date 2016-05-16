#-------------------------------------------------
#
# Project created by QtCreator 2016-01-25T12:04:15
#
#-------------------------------------------------
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MultiChannelViewer
TEMPLATE = app
#CONFIG += static



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


#Windows Libraries and Settings
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
        LIBS += -L$$PWD/lib/x86/win32/ -lavformat
        LIBS += -L$$PWD/lib/x86/win32/ -lavutil
        LIBS += -L$$PWD/lib/x86/win32/ -lswresample
        LIBS += -L$$PWD/lib/x86/win32/ -lswscale
    }
}

#OSX Libraries and Settings
macx
{
    contains(QT_ARCH, x86_64)
    {
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
        ICON = icon.icns

        QMAKE_LFLAGS += -F//System/Library/Frameworks
        QMAKE_LFLAGS += -lstdc++.6

        LIBS += -framework CoreFoundation
        LIBS += -framework CoreVideo
        LIBS += -framework VideoDecodeAcceleration
        LIBS += -framework CoreMedia
        LIBS += -framework VideoToolbox
        LIBS += -framework Security
        LIBS += -lz
        LIBS += -lbz2.1.0
        LIBS += -liconv

        INCLUDEPATH += $$PWD/lib/x64/osx/include/PvAPI
        DEPENDPATH += $$PWD/lib/x64/osx/include/PvAPI
        LIBS += -L$$PWD/lib/x64/osx -lPvAPI
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libPvAPI.a

        INCLUDEPATH += $$PWD/lib/x64/osx/include
        DEPENDPATH += $$PWD/lib/x64/osx/include

        LIBS += -L$$PWD/lib/x64/osx -lavcodec
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libavcodec.a

        LIBS += -L$$PWD/lib/x64/osx -lavformat
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libavformat.a

        LIBS += -L$$PWD/lib/x64/osx -lavutil
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libavutil.a

        LIBS += -L$$PWD/lib/x64/osx -lswscale
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libswscale.a

        LIBS += -L$$PWD/lib/x64/osx -lswresample
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libavcodec.a

        INCLUDEPATH += $$PWD/lib/x64/osx/include/x264
        DEPENDPATH += $$PWD/lib/x64/osx/include/x264

        LIBS += -L$$PWD/lib/x64/osx -lx264
        PRE_TARGETDEPS += $$PWD/lib/x64/osx/libx264.a
    }
}
