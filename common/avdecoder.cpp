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

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QSettings>

AVDecoder::AvInit init;

AVDecoder::AVDecoder() :
    pFormatCtx(NULL), pCodecCtx(NULL), av_stream(NULL), av_codec(NULL), stream_index(-1), codec_loaded(false)
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
    if(codec_loaded) {
        return true;
    }

    AVDictionary *opts = NULL;

    if((stream_index = av_find_best_stream(pFormatCtx, (AVMediaType)codec, -1, -1, &av_codec, 0)) < 0) {
        return false;
    }

    av_stream = pFormatCtx->streams[stream_index];
    pCodecCtx = av_stream->codec;

    if(avcodec_open2(pCodecCtx, av_codec, &opts) < 0) {
        codec_loaded = false;
        return false;
    }

    codec_loaded = true;
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
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,7,0)
        av_packet_unref(&packet);
#else
        av_free_packet(&packet);
#endif
    }

    if(frame_finished) {
        return pFrame;
    } else {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(&pFrame);
#else
        avcodec_free_frame(&pFrame);
#endif
        return NULL;
    }
}


bool AVDecoder::seekVideo(int percentage)
{
    if(!loadCodec(CODEC_VIDEO)) {
        return false;
    }

    qint64 seek_pos = pFormatCtx->duration * percentage / (AV_TIME_BASE * 100);
    qint64 frame = av_rescale(seek_pos, av_stream->time_base.den, av_stream->time_base.num);

    return avformat_seek_file(pFormatCtx, stream_index, 0, frame, frame, AVSEEK_FLAG_FRAME) >= 0;
}

static void calculate_thumbnail_dimensions(int src_width, int src_height,
                                           int src_sar_num, int src_sar_den,
                                           int &dst_width, int &dst_height) {

    if ((src_sar_num <= 0) || (src_sar_den <= 0)) {
        src_sar_num = 1;
        src_sar_den = 1;
    }

    if ((src_width * src_sar_num) / src_sar_den > src_height) {
        dst_width = 256;
        dst_height = (dst_width * src_height) / ((src_width * src_sar_num) / src_sar_den);
    } else {
        dst_height = 256;
        dst_width = (dst_height * ((src_width * src_sar_num) / src_sar_den)) / src_height;
    }

    if (dst_width < 8)
        dst_width = 8;

    if (dst_height < 1)
        dst_height = 1;
}

QByteArray AVDecoder::getThumbnail(int &width, int &height)
{
    QByteArray data;

    if(!loadCodec(CODEC_VIDEO)) {
        return data;
    }

    AVFrame *pFrame = getDecodedFrame(pCodecCtx, stream_index);

    if(pFrame != NULL) {

        calculate_thumbnail_dimensions(pCodecCtx->width, pCodecCtx->height,
                                       pCodecCtx->sample_aspect_ratio.num,
                                       pCodecCtx->sample_aspect_ratio.den,
                                       width, height);

        data = WriteJPEG(pCodecCtx, pFrame, width, height);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(&pFrame);
#else
        avcodec_free_frame(&pFrame);
#endif
    }

    return data;
}

QByteArray AVDecoder::WriteJPEG(AVCodecContext *pCodecCtx, AVFrame *pFrame, int width, int height)
{
    AVCodecContext *pOCodecCtx;
    AVCodec        *pOCodec;

    QByteArray data;

    pOCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);

    if (!pOCodec) {
        return data;
    }

    SwsContext *sws_ctx = sws_getContext(
                pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt,
                width, height,
                AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                NULL, NULL, NULL);

    if(!sws_ctx) {
        return data;
    }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
    AVFrame *pFrameRGB = av_frame_alloc();
#else
    AVFrame *pFrameRGB = avcodec_alloc_frame();
#endif

    if(pFrameRGB == NULL) {
        sws_freeContext(sws_ctx);
        return data;
    }

    // detect ffmpeg (>= 100) or libav (< 100)
#if (LIBAVUTIL_VERSION_MICRO >= 100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,63,100)) || \
    (LIBAVUTIL_VERSION_MICRO < 100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(54,6,0))
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 16);
#else
    int numBytes = avpicture_get_size(PIX_FMT_YUVJ420P, width, height);
#endif

    uint8_t *buffer = (uint8_t *)av_malloc(numBytes);

    if(!buffer) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(&pFrameRGB);
#else
        avcodec_free_frame(&pFrameRGB);
#endif
        sws_freeContext(sws_ctx);
        return data;
    }

    // detect ffmpeg (>= 100) or libav (< 100)
#if (LIBAVUTIL_VERSION_MICRO >= 100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,63,100)) || \
    (LIBAVUTIL_VERSION_MICRO < 100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(54,6,0))
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_YUV420P, width, height, 1);
#else
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_YUVJ420P, width, height);
#endif

    sws_scale(
        sws_ctx,
        pFrame->data,
        pFrame->linesize,
        0,
        pCodecCtx->height,
        pFrameRGB->data,
        pFrameRGB->linesize
    );

    pOCodecCtx = avcodec_alloc_context3(pOCodec);

    if(pOCodecCtx == NULL) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,52,0)
        avcodec_free_context(&pOCodecCtx);
#else
        avcodec_close(pOCodecCtx);
        av_free(pOCodecCtx);
#endif
        av_free(buffer);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(&pFrameRGB);
#else
        avcodec_free_frame(&pFrameRGB);
#endif
        sws_freeContext(sws_ctx);
        return  0;
    }

    pOCodecCtx->bit_rate      = pCodecCtx->bit_rate;
    pOCodecCtx->width         = width;
    pOCodecCtx->height        = height;
    pOCodecCtx->pix_fmt       = AV_PIX_FMT_YUVJ420P;
    pOCodecCtx->color_range   = AVCOL_RANGE_JPEG;
    pOCodecCtx->codec_id      = AV_CODEC_ID_MJPEG;
    pOCodecCtx->codec_type    = AVMEDIA_TYPE_VIDEO;
    pOCodecCtx->time_base.num = pCodecCtx->time_base.num;
    pOCodecCtx->time_base.den = pCodecCtx->time_base.den;

    AVDictionary *opts = NULL;
    if(avcodec_open2(pOCodecCtx, pOCodec, &opts) < 0) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,52,0)
        avcodec_free_context(&pOCodecCtx);
#else
        avcodec_close(pOCodecCtx);
        av_free(pOCodecCtx);
#endif
        av_free(buffer);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
        av_frame_free(&pFrameRGB);
#else
        avcodec_free_frame(&pFrameRGB);
#endif
        sws_freeContext(sws_ctx);
         return  0;
    }

    av_opt_set_int(pOCodecCtx, "lmin", pOCodecCtx->qmin * FF_QP2LAMBDA, 0);
    av_opt_set_int(pOCodecCtx, "lmax", pOCodecCtx->qmax * FF_QP2LAMBDA, 0);

    pOCodecCtx->mb_lmin        = pOCodecCtx->qmin * FF_QP2LAMBDA;
    pOCodecCtx->mb_lmax        = pOCodecCtx->qmax * FF_QP2LAMBDA;
    pOCodecCtx->flags          = AV_CODEC_FLAG_QSCALE;
    pOCodecCtx->global_quality = pOCodecCtx->qmin * FF_QP2LAMBDA;

    pFrame->pts     = 1;
    pFrame->quality = pOCodecCtx->global_quality;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    int gotPacket;

    avcodec_encode_video2(pOCodecCtx, &pkt, pFrameRGB, &gotPacket);

    QByteArray buffer2(reinterpret_cast<char *>(pkt.data), pkt.size);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,52,0)
        avcodec_free_context(&pOCodecCtx);
#else
        avcodec_close(pOCodecCtx);
        av_free(pOCodecCtx);
#endif
    av_free(buffer);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
    av_frame_free(&pFrameRGB);
#else
    avcodec_free_frame(&pFrameRGB);
#endif
    avcodec_close(pOCodecCtx);
    sws_freeContext(sws_ctx);

    return buffer2;
}

void AVDecoder::close()
{
    avformat_close_input(&pFormatCtx);
    pFormatCtx = NULL;
}
