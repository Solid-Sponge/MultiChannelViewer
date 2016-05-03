#-------------------------------------------------
#
# Project created by QtCreator 2016-01-25T12:04:15
#
#-------------------------------------------------
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MultiChannelViewer
TEMPLATE = app



SOURCES += main.cpp\
        multichannelviewer.cpp \
    camera.cpp \
    FFMPEGClass.cpp \
    autoexpose.cpp

HEADERS  += multichannelviewer.h \
    PvApi.h \
    PvRegIo.h \
    camera.h \
    FFMPEGClass.h \
    autoexpose.h


FORMS    += multichannelviewer.ui

DISTFILES += \
    README.md

macx: ICON = icon.icns



QMAKE_LFLAGS += -F//System/Library/Frameworks
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
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/locgal/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavdevice.a

macx: LIBS += -L/usr/local/lib -lavfilter
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavfilter.a

macx: LIBS += -L/usr/local/lib -lavutil
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavutil.a

macx: LIBS += -L/usr/local/lib -lswscale
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libswscale.a

macx: LIBS += -L/usr/local/lib -lswresample
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libswresample.a

macx: LIBS += -L/usr/local/lib -lavformat
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavformat.a

macx: LIBS += -L/usr/local/lib -lx264
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libx264.a

macx: LIBS += -L$$PWD/lib/x64/osx -lPvAPI
INCLUDEPATH += $$PWD/lib/x64/osx
DEPENDPATH += $$PWD/lib/x64/osx
macx: PRE_TARGETDEPS += $$PWD/lib/x64/osx/libPvAPI.a
