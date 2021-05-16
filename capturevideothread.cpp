#include "capturevideothread.h"
#include <QDebug>
#include <Windows.h>

void Yuv420EncodeH264(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt,
                   FILE* outfile);

CaptureVideoThread::CaptureVideoThread()
{
    pFormatCtxIn = NULL;
    pFormatCtxOut = NULL;
    pCodecCtx = NULL;
    rgb_buffer = NULL;
    out_buffer = NULL;
    mIsRun = false;
    mYuvFileName = fopen("out.yuv","wb");
    mH264FileName = fopen("out.h264","wb");
    //mOutFileName = "rtmp://localhost/live/room";
    mOutFileName = "rtmp://192.168.31.233:1935/live/room";
    qDebug() << "mOutFileName:" << mOutFileName << endl;

    //mMutex = new QMutex();
}

CaptureVideoThread::~CaptureVideoThread()
{
    deInit();
}

void av_error_string_output(const char* funcname, int line, int ret)
{
    char av_error[1024] = { '\0' };
    av_strerror(ret, av_error, 1024);
    qDebug() << "[av_error_string_output](" << funcname << ":" << line << ")" << av_error << endl;
}

void av_free_context(AVFormatContext* ictx, AVFormatContext* octx)
{
    if (NULL != ictx)
    {
        avformat_close_input(&ictx);
        avformat_free_context(ictx);
    }

    if (NULL != octx)
    {
        //avformat_close_input(&octx);
        avformat_free_context(octx);
    }
}

ErrorCode CaptureVideoThread::init(QString videoDevName)
{
    AVCodec *pCodec = NULL;
    pFormatCtxIn = avformat_alloc_context();
    //iFmt = av_find_input_format("dshow"); dshow时出错
    AVInputFormat *iFmt = av_find_input_format("vfwcap");
    AVDictionary* ioptions = NULL;
    int ret=0;

    av_dict_set(&ioptions, "framerate", "30", 0);
    if(avformat_open_input(&pFormatCtxIn,videoDevName.toUtf8(),iFmt,&ioptions) != 0)
    {
        qDebug() << "Can't open video device:" << videoDevName.toUtf8() << endl;
        return VideoOpenFailed;
    }
    av_dict_free(&ioptions);

    if(avformat_find_stream_info(pFormatCtxIn,NULL) < 0)
    {
        qDebug() << "Can't find stream information" << endl;
        return VideoStreamFindFailed;
    }
    av_dump_format(pFormatCtxIn, 0, videoDevName.toUtf8(), 0);

    videoIndex=-1;
    for(i=0;i<pFormatCtxIn->nb_streams;i++)
    {
       if(pFormatCtxIn->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
       {
           videoIndex=i;
           break;
       }
    }
    if(videoIndex == -1)
    {
        qDebug() << "Can't find video stream" << endl;
        return VideoStreamFindFailed;
    }

    pCodecCtx = pFormatCtxIn->streams[videoIndex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if(pCodec == NULL)
    {
        qDebug() << "Can't find video decoder" << endl;
        return AudioDecoderFindFailed;
    }

    if(avcodec_open2(pCodecCtx,pCodec,NULL) < 0)
    {
        printf("Can't open video decoder.\n");
        return AudioDecoderOpenFailed;
    }
    // h264部分 start
    AVCodec *pCodech264 = NULL;
    pCodech264 = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pCodech264) {
        qDebug() << "Ccodec not find";
        return AudioDecoderFindFailed;
    }
    pCodecCtxh264 = avcodec_alloc_context3(pCodech264);
    if (!pCodecCtxh264) {
        qDebug() << "Could not allocate video codec context";
        return AudioDecoderOpenFailed;
    }
    pCodecCtxh264->bit_rate = 400000;
    pCodecCtxh264->width = 640;
    pCodecCtxh264->height = 480;
    pCodecCtxh264->time_base.num=1;
    pCodecCtxh264->time_base.den=25;
    pCodecCtxh264->framerate = {25, 1};
    pCodecCtxh264->gop_size = 10;
    pCodecCtxh264->max_b_frames = 1;
    pCodecCtxh264->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtxh264->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //av_opt_set(pCodecCtxh264->priv_data, "preset", "slow", 0);
    //av_opt_set(pCodecCtxh264->priv_data, "tune", "zerolatency", 0);
    AVDictionary* param = NULL;
    //av_dict_set(&param, "preset", "superfast", 0);
    av_dict_set(&param, "preset", "slow", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);
    av_dict_set(&param, "profile", "main", 0);

    if (avcodec_open2(pCodecCtxh264, pCodech264, &param) < 0) {
        qDebug() << "Could not open codec encoder";
        return AudioDecoderOpenFailed;
    }
    av_dict_free(&param);
    // h264部分 stop

    // flv start
    ret = avformat_alloc_output_context2(&pFormatCtxOut, 0, "flv", mOutFileName);
    if (!pFormatCtxOut)
    {
        av_error_string_output(__FUNCTION__, __LINE__, ret);
        av_free_context(pFormatCtxIn, pFormatCtxOut);
        return AudioDecoderOpenFailed;
    }
    output_stream = avformat_new_stream(pFormatCtxOut, pCodecCtxh264->codec);
    if (NULL == output_stream)
    {
        av_error_string_output(__FUNCTION__, __LINE__, 0);
        av_free_context(pFormatCtxIn, pFormatCtxOut);
        return AudioDecoderOpenFailed;
    }

    ret = avcodec_parameters_from_context(output_stream->codecpar, pCodecCtxh264);
    if (0 != ret)
    {
        av_error_string_output(__FUNCTION__, __LINE__, ret);
        av_free_context(pFormatCtxIn, pFormatCtxOut);
        return AudioDecoderOpenFailed;
    }

    ret = avio_open(&pFormatCtxOut->pb, mOutFileName, AVIO_FLAG_WRITE);
    if (ret != 0)
    {
        av_error_string_output(__FUNCTION__, __LINE__, ret);
        avio_close(pFormatCtxOut->pb);
        av_free_context(pFormatCtxIn, pFormatCtxOut);
        return AudioDecoderOpenFailed;
    }

    av_dump_format(pFormatCtxOut, 0, mOutFileName, 1);

    ret = avformat_write_header(pFormatCtxOut, NULL);
    if (0 > ret)
    {
        av_error_string_output(__FUNCTION__, __LINE__, ret);
        avio_close(pFormatCtxOut->pb);
        av_free_context(pFormatCtxIn, pFormatCtxOut);
        return AudioDecoderOpenFailed;
    }

    return SUCCEED;
}

void CaptureVideoThread::deInit()
{
    if(out_buffer != NULL)
    {
        av_free(out_buffer);
        out_buffer = NULL;
    }
    if(rgb_buffer != NULL)
    {
        av_free(rgb_buffer);
        rgb_buffer = NULL;
    }
    if(pFrame != NULL)
    {
        av_free(pFrame);
        pFrame = NULL;
    }
    if(pFrameYuv != NULL)
    {
        av_free(pFrameYuv);
        pFrameYuv = NULL;
    }
    if(pFrameRGB != NULL)
    {
        av_free(pFrameRGB);
        pFrameRGB = NULL;
    }
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxh264);
    fclose(mYuvFileName);
    fclose(mH264FileName);
    av_free_context(pFormatCtxIn, pFormatCtxOut);
}

void CaptureVideoThread::startRecord()
{
    //mMutex->lock();
    mIsRun = true;
    this->start();
    //mMutex->unlock();
}

void CaptureVideoThread::stopRecord()
{
    //mMutex->lock();
    mIsRun = false;
    //mMutex->unlock();

    qDebug() << "stopRecord" << endl;
    //deInit();
    //this->deleteLater();
    //this->wait();

    //quit();
    //finished();
}

int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index){
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
        AV_CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
            NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame){
            ret=0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}


void CaptureVideoThread::run()
{
    int ret, got_frame;
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    AVPacket *packetOut = av_packet_alloc();
    av_init_packet(packetOut);
    pFrame = av_frame_alloc();
    pFrameYuv = av_frame_alloc();
    pFrameYuv->format = AV_PIX_FMT_YUV420P;
    pFrameYuv->width = pCodecCtx->width;
    pFrameYuv->height = pCodecCtx->height;
    int frameSize = avpicture_get_size(AV_PIX_FMT_YUV420P,
                                       pCodecCtx->width,
                                       pCodecCtx->height);
    out_buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*frameSize);
    avpicture_fill((AVPicture *)pFrameYuv,
                   out_buffer,
                   AV_PIX_FMT_YUV420P,
                   pCodecCtx->width,
                   pCodecCtx->height
                   );
    //qDebug() << "video fmt:" << pCodecCtx->pix_fmt << " " << AV_PIX_FMT_YUV420P;
    img_convert_ctx = sws_getContext(pCodecCtx->width,
                                     pCodecCtx->height,
                                     pCodecCtx->pix_fmt,
                                     pCodecCtx->width,
                                     pCodecCtx->height,
                                     AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC,
                                     NULL,
                                     NULL,
                                     NULL);

    pFrameRGB = av_frame_alloc();
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32,
                                      pCodecCtx->width,
                                      pCodecCtx->height);
    rgb_buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    avpicture_fill((AVPicture *)pFrameRGB,
                   rgb_buffer,
                   AV_PIX_FMT_RGB32,
                   pCodecCtx->width,
                   pCodecCtx->height);

    rgbimg_convert_ctx = sws_getContext(pCodecCtx->width,
                                        pCodecCtx->height,
                                        pCodecCtx->pix_fmt,
                                        pCodecCtx->width,
                                        pCodecCtx->height,
                                        AV_PIX_FMT_RGB32,
                                        SWS_BICUBIC,
                                        NULL,
                                        NULL,
                                        NULL);
    qDebug() << "width:" << pCodecCtx->width << ",height:" <<
                pCodecCtx->height << endl;
    int delayedFrame = 0;
    int loop = 0;
    qDebug() << "record video start" << endl;

    while(1)
    {

        if(!mIsRun)
            break;

        if(av_read_frame(pFormatCtxIn,packet) < 0)
        {
            qDebug() << "read video frame failed" << endl;
            msleep(10);
            continue;
        }

        if(packet->stream_index == videoIndex)
        {
            ret = avcodec_decode_video2(pCodecCtx,pFrame,&got_frame,packet);
            if(ret < 0)
            {
                qDebug() << "video decode failed" << endl;
            }

            if(got_frame)
            {
                sws_scale(img_convert_ctx,
                          (const uint8_t *const *)pFrame->data,
                          pFrame->linesize,
                          0,
                          pFrame->height,
                          pFrameYuv->data,
                          pFrameYuv->linesize
                        );

                sws_scale(rgbimg_convert_ctx,
                                  (const uint8_t *const *)pFrame->data,
                                  pFrame->linesize,
                                  0,
                                  pFrame->height,
                                  pFrameRGB->data,
                                  pFrameRGB->linesize
                                  );
                QImage tmpImg((uchar *)rgb_buffer,
                              pCodecCtx->width,
                              pCodecCtx->height,
                              QImage::Format_RGB32);
                //QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                emit sigGetOneImage(tmpImg);  //发送信号
                //qDebug() << "sigGetOneImage";

                int size = pCodecCtx->width * pCodecCtx->height;
                /*fwrite(pFrameYuv->data[0],1,size,mYuvFileName);
                fwrite(pFrameYuv->data[1],1,size/4,mYuvFileName);
                fwrite(pFrameYuv->data[2],1,size/4,mYuvFileName);
                */
                unsigned int untime = GetTickCount();
                pFrameYuv->pts = untime;
                //Yuv420EncodeH264(pCodecCtxh264,pFrameYuv,packetOut,mH264FileName);
                loop++;
                ret = avcodec_send_frame(pCodecCtxh264, pFrameYuv);
                if (ret < 0)
                    continue;
                ret = avcodec_receive_packet(pCodecCtxh264, packetOut);

                if (0 == ret)
                {
                    packetOut->stream_index = output_stream->index;
                    AVRational itime = pFormatCtxIn->streams[packet->stream_index]->time_base;
                    AVRational otime = pFormatCtxOut->streams[packet->stream_index]->time_base;

                    packetOut->pts = av_rescale_q_rnd(packet->pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    packetOut->dts = av_rescale_q_rnd(packet->dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    packetOut->duration = av_rescale_q_rnd(packet->duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                    packetOut->pos = -1;

                    ret = av_interleaved_write_frame(pFormatCtxOut, packetOut);
                    qDebug() << "output frame " << loop - delayedFrame << endl;
                }
                else {
                    delayedFrame++;
                    qDebug() << "output no frame" << endl;;
                }
                av_free_packet(packetOut);
            }

        }

        av_free_packet(packet);

    }

    //flush_encoder(pFormatCtxIn,videoIndex);
    //flush_encoder(pFormatCtxOut,videoIndex);

    qDebug() << "record video stop 111" << endl;
    //deInit();
    qDebug() << "record video stop 222" << endl;
    //terminate();
    qDebug() << "record video stop 333" << endl;
}

void Yuv420EncodeH264(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt,
                   FILE* outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        printf("Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            printf("Error during encoding\n");
            exit(1);
        }

        qDebug() << "Write packet:" << pkt->size <<  " size=:" << pkt->pts;
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}
