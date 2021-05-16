# RTMP-push-stream-based-on-Qt-and-FFMPEG
客户端：多线程实现摄像头的推流以及显示，子线程使用ffmpeg采集本地摄像头数据，然后将其转码为YUV420，接着将YUV420再转码为RGB32，
使用H264编码器将YUV420进行压缩编码，最后通过网络地址实现flv推流。子线程得到一帧数据后发送信号给主线程，主线程重载paintEvent
函数实现摄像头数据显示。
服务器：Linux安装ffmpeg、nginx及librtmp，使用ffplay播放rtmp网络流数据
