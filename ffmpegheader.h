#ifndef FFMPEGHEADER_H
#define FFMPEGHEADER_H

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libavdevice/avdevice.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
    #include "libavutil/opt.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif

enum ErrorCode
{
    SUCCEED,
    AudioOpenFailed,
    VideoOpenFailed,
    AudioStreamFindFailed,
    VideoStreamFindFailed,
    AudioDecoderFindFailed,
    VideoDecoderFindFailed,
    AudioDecoderOpenFailed,
    VideoDecoderOpenFailed,
};

typedef unsigned char uint8_t;

#endif // FFMPEGHEADER_H

