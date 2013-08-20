#ifndef AVDECODER_H
#define AVDECODER_H

#include <QImage>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <vitamtp.h>
}

class AVDecoder
{
public:
    AVDecoder();
    ~AVDecoder();
    bool open(const QString filename);
    QByteArray getAudioThumbnail(int width, int height);
    QByteArray getVideoThumbnail(int width, int height);
    void getPictureMetadata(metadata_t &metadata);
    void getAudioMetadata(metadata_t &metadata);
    void getVideoMetadata(metadata_t &metadata);
    void close();

    static void init();

private:
    void AVFrameToQImage(AVFrame &frame, QImage &image, int width, int height);
    AVFrame *getDecodedFrame(AVCodecContext *pCodecCtx, int stream_index);

    AVFormatContext *pFormatCtx;
};

#endif // AVDECODER_H
