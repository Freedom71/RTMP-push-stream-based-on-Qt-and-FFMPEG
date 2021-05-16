#-------------------------------------------------
#
# Project created by QtCreator 2021-04-21T17:25:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpeg_camera_show_rtmp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    capturevideothread.cpp

HEADERS  += mainwindow.h \
    capturevideothread.h \
    ffmpegheader.h

FORMS    += mainwindow.ui

INCLUDEPATH += D:/ffmpeg/ffmpeg-4.2.1-win32-dev/include

LIBS += D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/avcodec.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/avdevice.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/avfilter.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/avformat.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/avutil.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/postproc.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/swresample.lib \
        D:/ffmpeg/ffmpeg-4.2.1-win32-dev/lib/swscale.lib
