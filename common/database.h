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

#ifndef DATABASE_H
#define DATABASE_H

#include <QMutex>
#include <QObject>

#include <vitamtp.h>

typedef struct {
    const char *file_ext;
    int file_format;
    int file_codec;
} file_type;

#define OHFI_BASE_VALUE 256

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

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = 0);

    virtual bool load() = 0;
    virtual bool rescan() = 0;
    virtual void setUUID(const QString &uuid) = 0;

    virtual int childObjectCount(int parent_ohfi) = 0;
    virtual bool deleteEntry(int ohfi, int root_ohfi = 0) = 0;
    virtual QString getAbsolutePath(int ohfi) = 0;
    virtual QString getRelativePath(int ohfi) = 0;
    virtual bool getObjectList(int ohfi, metadata_t **metadata) = 0;
    virtual bool getObjectMetadata(int ohfi, metadata_t &metadata) = 0;
    virtual int getObjectMetadatas(int parent_ohfi, metadata_t **metadata, int index = 0, int max_number = 0) = 0;
    virtual qint64 getObjectSize(int ohfi) = 0;
    virtual int getPathId(const char *name, int ohfi) = 0;
    virtual int insertObjectEntry(const QString &path, const QString &name, int parent_ohfi) = 0;
    virtual bool renameObject(int ohfi, const QString &name) = 0;
    virtual void setObjectSize(int ohfi, qint64 size) = 0;
    virtual int getParentId(int ohfi) = 0;
    virtual int getRootId(int ohfi) = 0;
    virtual void freeMetadata(metadata_t *metadata) = 0;

    static int checkFileType(const QString path, int ohfi_root);
    static void loadMusicMetadata(const QString &path, metadata_t &metadata);
    static void loadPhotoMetadata(const QString &path, metadata_t &metadata);
    static void loadVideoMetadata(const QString &path, metadata_t &metadata);

    QMutex mutex;

protected:
    bool continueOperation();

private:

    virtual void clear() = 0;
    virtual int create() = 0;

    // control variables
    QMutex cancel;
    bool cancel_operation;

signals:
    void fileAdded(QString);
    void directoryAdded(QString);
    void updated(int);

protected slots:
    void process();

public slots:
    void cancelOperation();
};

#endif // DATABASE_H
