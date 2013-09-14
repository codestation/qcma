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

#ifndef CMAOBJECT_H
#define CMAOBJECT_H

#include <QFileInfo>
#include <QString>

#include <vitamtp.h>

#define OHFI_OFFSET 256

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

class CMAObject
{
public:
    explicit CMAObject(CMAObject *parent = 0);
    ~CMAObject();

    void refreshPath();
    bool removeReferencedObject();
    void rename(const QString &name);
    void updateObjectSize(qint64 size);
    bool hasParent(const CMAObject *obj);
    void initObject(const QFileInfo &file, int file_type = -1);

    bool operator==(const CMAObject &obj);
    bool operator!=(const CMAObject &obj);
    bool operator<(const CMAObject &obj);

    inline void setOhfi(int ohfi) {
        metadata.ohfi = ohfi;
    }

    inline static void resetOhfiCounter() {
        ohfi_count = OHFI_OFFSET;
    }

    typedef struct {
        const char *file_ext;
        int file_format;
        int file_codec;
    } file_type;

    static const file_type audio_list[3];
    static const file_type photo_list[7];
    static const char *video_list[1];

    QString path;
    CMAObject *parent;
    metadata_t metadata;

protected:
    static int ohfi_count;

private:
    void loadSfoMetadata(const QString &path);
    void loadMusicMetadata(const QString &path);
    void loadVideoMetadata(const QString &path);
    void loadPhotoMetadata(const QString &path);
};

#endif // CMAOBJECT_H
