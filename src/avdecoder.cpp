/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2013  Codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "avdecoder.h"
#include "cmaobject.h"

#include <QDebug>
#include <QBuffer>
#include <QFile>
#include <QSettings>

AVDecoder::AvInit init;

AVDecoder::AVDecoder() :
    pFormatCtx(NULL)
{
}

AVDecoder::~AVDecoder()
{
    if(pFormatCtx) {
        avformat_close_input(&pFormatCtx);
    }
}

bool AVDecoder::open(const QString filename)
{
    if(avformat_open_input(&pFormatCtx, QFile::encodeName(filename).constData(), NULL, NULL) != 0) {
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

    metadata.data.music.tracks->data.track_audio.bitrate = pFormatCtx->bit_rate;
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
        metadata.data.video.title = strdup(metadata.name);
    }

    metadata.data.video.tracks->data.track_video.duration = pFormatCtx->duration / 1000;
    metadata.data.video.tracks->data.track_video.bitrate = pFormatCtx->bit_rate;

    int stream_index;
    AVCodec *codec = NULL;

    if((stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) >= 0) {
        AVCodecContext *pCodecCtx = pFormatCtx->streams[stream_index]->codec;
        metadata.data.video.tracks->data.track_video.width = pCodecCtx->width;
        metadata.data.video.tracks->data.track_video.height = pCodecCtx->height;
        if(strcmp(codec->name, "h264") == 0) {
            metadata.data.video.tracks->data.track_video.codecType = CODEC_TYPE_AVC;
        } else if(strcmp(codec->name, "mpeg4") == 0) {
            metadata.data.video.tracks->data.track_video.codecType = CODEC_TYPE_MPEG4;
        } else {
            metadata.data.video.tracks->data.track_video.codecType = 0;
        }
    }
}

QByteArray AVDecoder::getAudioThumbnail(int width, int height)
{
    QByteArray data;

    int stream_index;
    AVCodec *codec = NULL;
    if((stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0) {
        // no thumbnail
        return data;
    }

    AVPacket pkt;
    if(av_read_frame(pFormatCtx, &pkt) >= 0) {
        // first frame == first thumbnail (hopefully)
        QBuffer imgbuffer(&data);
        imgbuffer.open(QIODevice::WriteOnly);
        QImage img = QImage::fromData(QByteArray((const char *)pkt.data, pkt.size));
        QImage result = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        result.save(&imgbuffer, "JPEG");
    }

    return data;
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
    AVDictionary *opts = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *pCodecCtx = NULL;

    int percentage = QSettings().value("videoThumbnailSeekPercentage", 30).toInt();

    if((stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0) {
        return data;
    }

    pCodecCtx = pFormatCtx->streams[stream_index]->codec;

    if(avcodec_open2(pCodecCtx, codec, &opts) < 0) {
        avcodec_close(pCodecCtx);
        return data;
    }

    qint64 seek_pos = pFormatCtx->duration * percentage / (AV_TIME_BASE * 100);
    qint64 frame = av_rescale(seek_pos,pFormatCtx->streams[stream_index]->time_base.den, pFormatCtx->streams[stream_index]->time_base.num);

    if(avformat_seek_file(pFormatCtx, stream_index, 0, frame, frame, AVSEEK_FLAG_FRAME) < 0) {
        avcodec_close(pCodecCtx);
        return data;
    }

    if((pFrame = getDecodedFrame(pCodecCtx, stream_index)) == NULL) {
        avcodec_close(pCodecCtx);
        return data;
    }

    AVFrame *pFrameRGB = avcodec_alloc_frame();

    int numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes);

    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    SwsContext *sws_ctx = sws_getContext(
                              pCodecCtx->width,
                              pCodecCtx->height,
                              pCodecCtx->pix_fmt,
                              pCodecCtx->width,
                              pCodecCtx->height,
                              PIX_FMT_RGB24,
                              SWS_BICUBIC,
                              NULL,
                              NULL,
                              NULL
                          );

    if(!sws_ctx) {
        avcodec_close(pCodecCtx);
        return data;
    }

    sws_scale(
        sws_ctx,
        pFrame->data,
        pFrame->linesize,
        0,
        pCodecCtx->height,
        pFrameRGB->data,
        pFrameRGB->linesize
    );

    QImage image(pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB888);

    for(int y = 0, h = pCodecCtx->height, w = pCodecCtx->width; y < h; y++) {
        memcpy(image.scanLine(y), pFrameRGB->data[0] + y * pFrameRGB->linesize[0], w * 3);
    }

    QBuffer imgbuffer(&data);
    imgbuffer.open(QIODevice::WriteOnly);
    QImage result = image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    pFormatCtx = NULL;
}
