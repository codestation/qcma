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

#include "utils.h"
#include "avdecoder.h"

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QImage>

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

const file_type audio_list[] = {
    {"mp3", FILE_FORMAT_MP3, CODEC_TYPE_MP3},
    {"mp4", FILE_FORMAT_MP4, CODEC_TYPE_AAC},
    {"wav", FILE_FORMAT_WAV, CODEC_TYPE_PCM}
};

const file_type photo_list[] = {
    {"jpg",  FILE_FORMAT_JPG, CODEC_TYPE_JPG},
    {"jpeg", FILE_FORMAT_JPG, CODEC_TYPE_JPG},
    {"png",  FILE_FORMAT_PNG, CODEC_TYPE_PNG},
    {"tif",  FILE_FORMAT_TIF, CODEC_TYPE_TIF},
    {"tiff", FILE_FORMAT_TIF, CODEC_TYPE_TIF},
    {"bmp",  FILE_FORMAT_BMP, CODEC_TYPE_BMP},
    {"gif",  FILE_FORMAT_GIF, CODEC_TYPE_GIF},
};

const file_type video_list[] = {
    {"mp4", FILE_FORMAT_MP4, 0}
};

bool getDiskSpace(const QString &dir, quint64 *free, quint64 *total)
{
#ifdef Q_OS_WIN32

    if(GetDiskFreeSpaceEx(dir.toStdWString().c_str(), (ULARGE_INTEGER *)free, (ULARGE_INTEGER *)total, NULL) != 0) {
        return true;
    }

#else
    struct statvfs stat;

    if(statvfs(dir.toUtf8().data(), &stat) == 0) {
        *total = stat.f_frsize * stat.f_blocks;
        *free = stat.f_frsize * stat.f_bfree;
        return true;
    }

#endif
    return false;
}

bool removeRecursively(const QString &dirName)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QDir(dirName).removeRecursively();
#else
    bool result = false;
    QDir dir(dirName);

    if(dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if(info.isDir()) {
                result = removeRecursively(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if(!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
#endif
}

QByteArray findFolderAlbumArt(const QString path, metadata_t *metadata)
{
    QByteArray data;
    QDir folder(path);

    QStringList files = folder.entryList(QDir::Files | QDir::Readable);
    const QStringList cover_list = QStringList() << "album" << "cover" << "front";
    const QStringList ext_list = QStringList() << "jpg" << "jpeg" << "png" << "gif";

    foreach(const QString &file, files) {
        foreach(const QString &cover, cover_list) {
            foreach(const QString &ext, ext_list) {
                if(file.compare(QString("%1.%2").arg(cover, ext), Qt::CaseInsensitive) == 0) {
                    qDebug() << "Trying to load album art from" << folder.absoluteFilePath(file);
                    QImage img;
                    if(img.load(folder.absoluteFilePath(file))) {
                        QBuffer buffer(&data);
                        buffer.open(QIODevice::WriteOnly);
                        QImage result = img.scaled(256, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        result.save(&buffer, "JPEG");
                        metadata->data.thumbnail.width = result.width();
                        metadata->data.thumbnail.height = result.height();
                    }
                    // only try with the first match
                    break;
                }
            }
        }
    }
    return data;
}

QByteArray getThumbnail(const QString &path, DataType type, metadata_t *metadata)
{
    QByteArray data;

    if(MASK_SET(type, SaveData)) {
        QFile file(QDir(path).absoluteFilePath("ICON0.PNG"));

        if(file.open(QIODevice::ReadOnly)) {
            data = file.readAll();
        }
    } else if(MASK_SET(type, Photo)) {
        QImage img;

        if(img.load(path)) {
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QImage result = img.scaled(240, 136, Qt::KeepAspectRatio, Qt::FastTransformation);
            result.save(&buffer, "JPEG");
            metadata->data.thumbnail.width = result.width();
            metadata->data.thumbnail.height = result.height();
        }
    } else if(MASK_SET(type, Music)) {
        if(MASK_SET(type, Folder)) {
            // TODO: try to load an album cover from one of the audio files.
            data = findFolderAlbumArt(path, metadata);
        } else {
            AVDecoder decoder;

            if(decoder.open(path)) {
                data = decoder.getAudioThumbnail(256, 256);
                metadata->data.thumbnail.width = 256;
                metadata->data.thumbnail.height = 256;
            }
        }
    }  else if(MASK_SET(type, Video)) {
        AVDecoder decoder;
        if(decoder.open(path)) {
            data = decoder.getVideoThumbnail(256, 256);
            metadata->data.thumbnail.width = 256;
            metadata->data.thumbnail.height = 256;
        }
    }
    return data;
}

QString readable_size(quint64 size, bool use_gib)
{
    QStringList list;
    list << "KiB" << "MiB";
    if(use_gib) {
        list << "GiB";
    }

    QStringListIterator i(list);
    QString unit("bytes");

    float size_f = size;

    while(size_f >= 1024.0 && i.hasNext()) {
        unit = i.next();
        size_f /= 1024.0;
    }
    return QString().setNum(size_f,'f',2) + " " + unit;
}

int checkFileType(const QString path, int ohfi_root)
{
    switch(ohfi_root) {
    case VITA_OHFI_MUSIC:
        for(int i = 0, max = sizeof(audio_list) / sizeof(file_type); i < max; i++) {
            if(path.endsWith(audio_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    case VITA_OHFI_PHOTO:
        for(int i = 0, max = sizeof(photo_list) / sizeof(file_type); i< max; i++) {
            if(path.endsWith(photo_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    case VITA_OHFI_VIDEO:
        for(int i = 0, max = sizeof(video_list) / sizeof(file_type); i< max; i++) {
            if(path.endsWith(video_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    default:
        return 0;
    }
    return -1;
}
