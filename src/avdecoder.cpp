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

#include "cmautils.h"
#include "avdecoder.h"
#include "database.h"

#include <QDebug>
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QSettings>

AVDecoder::AvInit init;

AVDecoder::AVDecoder() :
    pFormatCtx(NULL), pCodecCtx(NULL), av_stream(NULL), av_codec(NULL), stream_index(-1)
{
}

AVDecoder::~AVDecoder()
{
    if(pCodecCtx) {
        avcodec_close(pCodecCtx);
    }
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

bool AVDecoder::loadCodec(codec_type codec)
{
    AVDictionary *opts = NULL;

    if((stream_index = av_find_best_stream(pFormatCtx, (AVMediaType)codec, -1, -1, &av_codec, 0)) < 0) {
        return false;
    }

    av_stream = pFormatCtx->streams[stream_index];
    pCodecCtx = av_stream->codec;

    if(avcodec_open2(pCodecCtx, av_codec, &opts) < 0) {
        return false;
    }

    return true;
}

const char *AVDecoder::getMetadataEntry(const char *key, const char *default_value)
{
    AVDictionaryEntry *entry;

    if((entry = av_dict_get(pFormatCtx->metadata, key, NULL, 0)) != NULL) {
        return entry->value;
    }
    return default_value;
}

void AVDecoder::getAudioMetadata(metadata_t &metadata)
{
    metadata.data.music.artist = strdup(getMetadataEntry("artist", ""));
    metadata.data.music.album = strdup(getMetadataEntry("album", ""));
    metadata.data.music.title = strdup(getMetadataEntry("title", ""));

    if(loadCodec(CODEC_AUDIO)) {
        metadata.data.music.tracks->data.track_audio.bitrate = pCodecCtx->bit_rate;
    } else {
        metadata.data.music.tracks->data.track_audio.bitrate = pFormatCtx->bit_rate;
    }
}

void AVDecoder::getVideoMetadata(metadata_t &metadata)
{
    metadata.data.video.copyright = strdup(getMetadataEntry("copyright", ""));
    metadata.data.video.explanation = strdup(getMetadataEntry("comments", ""));
    metadata.data.video.title = strdup(getMetadataEntry("title", metadata.name));

    if(loadCodec(CODEC_VIDEO)) {
        metadata.data.video.tracks->data.track_video.width = pCodecCtx->width;
        metadata.data.video.tracks->data.track_video.height = pCodecCtx->height;
        metadata.data.video.tracks->data.track_video.bitrate = pCodecCtx->bit_rate;
        metadata.data.video.tracks->data.track_video.duration = pFormatCtx->duration / 1000;

        if(strcmp(av_codec->name, "h264") == 0) {
            metadata.data.video.tracks->data.track_video.codecType = CODEC_TYPE_AVC;
        } else if(strcmp(av_codec->name, "mpeg4") == 0) {
            metadata.data.video.tracks->data.track_video.codecType = CODEC_TYPE_MPEG4;
        } else {
            metadata.data.video.tracks->data.track_video.codecType = 0;
        }
    }
}

QByteArray AVDecoder::getAudioThumbnail(int width, int height)
{
    QByteArray data;

    if(!loadCodec(CODEC_VIDEO)) {
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

AVFrame *AVDecoder::getDecodedFrame(AVCodecContext *codec_ctx, int frame_stream_index)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
    AVFrame *pFrame = av_frame_alloc();
#else
    AVFrame *pFrame = avcodec_alloc_frame();
#endif

    AVPacket packet;
    int frame_finished = 0;

    while(!frame_finished && av_read_frame(pFormatCtx, &packet)>=0) {
        if(packet.stream_index == frame_stream_index) {
            avcodec_decode_video2(codec_ctx, pFrame, &frame_finished, &packet);
        }
        av_free_packet(&packet);
    }

    if(frame_finished) {
        return pFrame;
    } else {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(pFrame);
#else
        av_free(pFrame);
#endif
        return NULL;
    }
}

QByteArray AVDecoder::getVideoThumbnail(int width, int height)
{
    QByteArray data;
    AVFrame *pFrame;

    int percentage = QSettings().value("videoThumbnailSeekPercentage", 30).toInt();

    if(!loadCodec(CODEC_VIDEO)) {
        return data;
    }

    qint64 seek_pos = pFormatCtx->duration * percentage / (AV_TIME_BASE * 100);
    qint64 frame = av_rescale(seek_pos, av_stream->time_base.den, av_stream->time_base.num);

    if(avformat_seek_file(pFormatCtx, stream_index, 0, frame, frame, AVSEEK_FLAG_FRAME) < 0) {
        avcodec_close(pCodecCtx);
        return data;
    }

    if((pFrame = getDecodedFrame(pCodecCtx, stream_index)) == NULL) {
        avcodec_close(pCodecCtx);
        return data;
    }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
    AVFrame *pFrameRGB = av_frame_alloc();
#else
    AVFrame *pFrameRGB = avcodec_alloc_frame();
#endif

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

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
    av_frame_free(pFrameRGB);
    av_frame_free(pFrame);
#else
    av_free(pFrameRGB);
    av_free(pFrame);
#endif

    avcodec_close(pCodecCtx);

    return data;
}

void AVDecoder::close()
{
    avformat_close_input(&pFormatCtx);
    pFormatCtx = NULL;
}
