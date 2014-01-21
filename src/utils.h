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

#ifndef UTILS_H
#define UTILS_H

#include <QByteArray>
#include <QString>
#include <QThread>

#include <vitamtp.h>

typedef struct {
    const char *file_ext;
    int file_format;
    int file_codec;
} file_type;

#define FILE_FORMAT_MP4 1
#define FILE_FORMAT_WAV 2
#define FILE_FORMAT_MP3 3
#define FILE_FORMAT_JPG 4
#define FILE_FORMAT_PNG 5
#define FILE_FORMAT_GIF 6
#define FILE_FORMAT_BMP 7
#define FILE_FORMAT_TIF 8

#define CODEC_TYPE_MPEG4 2
#define CODEC_TYPE_AVC 3
#define CODEC_TYPE_MP3 12
#define CODEC_TYPE_AAC 13
#define CODEC_TYPE_PCM 15
#define CODEC_TYPE_JPG 17
#define CODEC_TYPE_PNG 18
#define CODEC_TYPE_TIF 19
#define CODEC_TYPE_BMP 20
#define CODEC_TYPE_GIF 21

extern const file_type audio_list[3];
extern const file_type photo_list[7];
extern const file_type video_list[1];

// Qt4 doesn't have public methods for Thread::*sleep
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
typedef QThread Sleeper;
#else
class Sleeper : QThread
{
public:
    static void sleep(unsigned long secs) {
        QThread::sleep(secs);
    }
    static void msleep(unsigned long msecs) {
        QThread::msleep(msecs);
    }
    static void usleep(unsigned long usecs) {
        QThread::usleep(usecs);
    }
};
#endif

bool removeRecursively(const QString &dirName);
QString readable_size(quint64 size, bool use_gib = false);
bool getDiskSpace(const QString &dir, quint64 *free, quint64 *total);
QByteArray getThumbnail(const QString &path, DataType type, metadata_t *metadata);
int checkFileType(const QString path, int ohfi_root);

#endif // UTILS_H
