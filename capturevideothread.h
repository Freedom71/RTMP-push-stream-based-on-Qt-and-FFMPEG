#ifndef CAPTUREVIDEOTHREAD_H
#define CAPTUREVIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include "ffmpegheader.h"

class CaptureVideoThread : public QThread
{
    Q_OBJECT
public:
    CaptureVideoThread();
    ~CaptureVideoThread();

    ErrorCode init(QString videoDevName);
    void deInit();
    void startRecord();
    void stopRecord();
signals:
    void sigGetOneImage(QImage img);
protected:
    void run();
private:
    AVFormatContext *pFormatCtxIn;
    AVFormatContext *pFormatCtxOut;
    int i,videoIndex;
    AVCodecContext *pCodecCtx;
    AVCodecContext *pCodecCtxh264;
    AVFrame *pFrame;
    AVFrame *pFrameYuv;
    AVFrame *pFrameRGB;
    uint8_t *out_buffer;
    uint8_t *rgb_buffer;
    AVStream* output_stream;
    struct SwsContext *img_convert_ctx;
    struct SwsContext *rgbimg_convert_ctx;
    bool mIsRun;
    FILE *mYuvFileName;
    FILE *mH264FileName;
    char *mOutFileName;
    QMutex *mMutex;
};

#endif // CAPTUREVIDEOTHREAD_H
