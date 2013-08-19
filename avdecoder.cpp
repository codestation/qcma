#include "avdecoder.h"

#include <QBuffer>
#include <QSettings>

AVDecoder::AVDecoder() :
    pFormatCtx(NULL)
{
}

bool AVDecoder::open(const QString filename)
{
    if(avformat_open_input(&pFormatCtx, filename.toStdString().c_str(), NULL, NULL) != 0) {
        return false;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        avformat_close_input(&pFormatCtx);
        return false;
    }

    return true;
}

void AVDecoder::getAudioMetadata(metadata_t &metadata)
{
    AVDictionaryEntry *entry;
    AVDictionary* file_metadata = pFormatCtx->metadata;

    if((entry = av_dict_get(file_metadata, "artist", NULL, 0)) != NULL) {
        metadata.data.music.artist = strdup(entry->value);
    } else {
        metadata.data.music.artist = strdup("");
    }

    if((entry = av_dict_get(file_metadata, "album", NULL, 0)) != NULL) {
        metadata.data.music.album = strdup(entry->value);
    } else {
        metadata.data.music.album = strdup("");
    }

    if((entry = av_dict_get(file_metadata, "title", NULL, 0)) != NULL) {
        metadata.data.music.title = strdup(entry->value);
    } else {
        metadata.data.music.title = strdup("");
    }

    av_dict_free(&file_metadata);
}

void AVDecoder::getVideoMetadata(metadata_t &metadata)
{
    AVDictionaryEntry *entry;
    AVDictionary* file_metadata = pFormatCtx->metadata;

    if((entry = av_dict_get(file_metadata, "copyright", NULL, 0)) != NULL) {
        metadata.data.video.copyright = strdup(entry->value);
    } else {
        metadata.data.video.copyright = strdup("");
    }

    if((entry = av_dict_get(file_metadata, "comments", NULL, 0)) != NULL) {
        metadata.data.video.explanation = strdup(entry->value);
    } else {
        metadata.data.video.explanation = strdup("");
    }

    if((entry = av_dict_get(file_metadata, "title", NULL, 0)) != NULL) {
        metadata.data.video.title = strdup(entry->value);
    } else {
        metadata.data.video.title = strdup("");
    }

    av_dict_free(&file_metadata);
}

QByteArray AVDecoder::getAudioThumbnail(int width, int height)
{
    QByteArray data;
    for (uint i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket pkt = pFormatCtx->streams[i]->attached_pic;

            QBuffer imgbuffer(&data);
            imgbuffer.open(QIODevice::WriteOnly);
            QImage img = QImage::fromData(QByteArray((const char *)pkt.data, pkt.size));
            QImage result = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            result.save(&imgbuffer, "JPEG");
            av_free_packet(&pkt);
            break;
        }
    }
    return data;
}

void AVDecoder::AVFrameToQImage(AVFrame &frame, QImage &image, int width, int height)
{
    quint8 *src = frame.data[0];

    for (int y = 0; y < height; y++) {
        QRgb *scanLine = (QRgb *)image.scanLine(y);

        for (int x = 0; x < width; x++) {
            scanLine[x] = qRgb(src[3*x], src[3*x+1], src[3*x+2]);
        }
        src += frame.linesize[0];
    }
}

AVFrame *AVDecoder::getDecodedFrame(AVCodecContext *pCodecCtx, int stream_index)
{
    AVFrame *pFrame = avcodec_alloc_frame();

    AVPacket packet;
    int frame_finished = 0;

    while(!frame_finished && av_read_frame(pFormatCtx, &packet)>=0) {
        if(packet.stream_index == stream_index) {
            avcodec_decode_video2(pCodecCtx, pFrame, &frame_finished, &packet);
        }
        av_free_packet(&packet);
    }

    if(frame_finished) {
        return pFrame;
    } else {
        av_free(pFrame);
        return NULL;
    }
}

QByteArray AVDecoder::getVideoThumbnail(int width, int height)
{
    QByteArray data;
    int stream_index;
    AVFrame *pFrame;
    AVCodec *codec = NULL;
    AVCodecContext *pCodecCtx = NULL;

    int percentage = QSettings().value("videoThumbnailSeekPercentage", 30).toInt();

    if((stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0) {
        return data;
    }

    pCodecCtx = pFormatCtx->streams[stream_index]->codec;

    if(avcodec_open2(pCodecCtx, codec, NULL) < 0) {
        avcodec_close(pCodecCtx);
        return data;
    }

    if(av_seek_frame(pFormatCtx, stream_index, pFormatCtx->duration * percentage / 100, 0) < 0) {
        avcodec_close(pCodecCtx);
        return data;
    }

    if((pFrame = getDecodedFrame(pCodecCtx, stream_index)) == NULL) {
        avcodec_close(pCodecCtx);
        return data;
    }

    AVFrame *pFrameRGB = avcodec_alloc_frame();

    int numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    SwsContext *sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    QImage img(pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);

    sws_scale(
        sws_ctx,
        (uint8_t const * const *)pFrame->data,
        pFrame->linesize,
        0,
        pCodecCtx->height,
        pFrameRGB->data,
        pFrameRGB->linesize
    );

    AVFrameToQImage(*pFrame, img, pCodecCtx->width, pCodecCtx->height);

    QBuffer imgbuffer(&data);
    imgbuffer.open(QIODevice::WriteOnly);
    QImage result = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    result.save(&imgbuffer, "JPEG");

    av_free(buffer);
    av_free(pFrameRGB);
    av_free(pFrame);

    avcodec_close(pCodecCtx);

    return data;
}

void AVDecoder::close()
{
    avformat_close_input(&pFormatCtx);
}
