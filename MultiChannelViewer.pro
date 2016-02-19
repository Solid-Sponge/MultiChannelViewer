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
    FFMPEGClass.cpp

HEADERS  += multichannelviewer.h \
    PvApi.h \
    PvRegIo.h \
    camera.h \
    FFMPEGClass.h


FORMS    += multichannelviewer.ui

DISTFILES += \
    README.md

macx: ICON = icon.icns


#macx: LIBS += -L$$PWD/./ -lPvAPI

#INCLUDEPATH += $$PWD/.
#DEPENDPATH += $$PWD/.

macx: LIBS += -L$$PWD -lPvAPI



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
#macx: LIBS += -lswresample



macx: LIBS += -L/usr/local/lib -lavcodec
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
macx: PRE_TARGETDEPS += /usr/local/lib/libavcodec.a

macx: LIBS += -L/usr/local/lib -lavdevice
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
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
