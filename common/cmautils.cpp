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

#ifdef FFMPEG_ENABLED
#include "avdecoder.h"
#endif

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QSettings>

#ifdef Q_OS_WIN32
#include <windows.h>
#elif defined Q_OS_ANDROID
#include <sys/vfs.h>
#define statvfs statfs
#else
#include <sys/statvfs.h>
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#endif

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

bool removeRecursively(const QString &path)
{
    QFileInfo file_info(path);

    if(file_info.isDir()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        return QDir(path).removeRecursively();
#else
        bool result = false;
        QDir dir(path);

        QDir::Filters filter = QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files;

        if(dir.exists(path)) {
            foreach(QFileInfo info, dir.entryInfoList(filter, QDir::DirsFirst)) {
                if(info.isDir()) {
                    result = removeRecursively(info.absoluteFilePath());
                } else {
                    result = QFile::remove(info.absoluteFilePath());
                }

                if(!result) {
                    return result;
                }
            }
            result = dir.rmdir(path);
        }

        return result;
#endif
    } else {
        return QFile::remove(path);
    }
}

static QByteArray findFolderAlbumArt(const QString path, metadata_t *metadata)
{
    QByteArray data;

#ifndef FFMPEG_ENABLED
    Q_UNUSED(path);
    Q_UNUSED(metadata);
#else
    QDir folder(path);

    QStringList files = folder.entryList(QDir::Files | QDir::Readable);
    const QStringList cover_list = QStringList() << "album" << "cover" << "front";
    const QStringList ext_list = QStringList() << "jpg" << "jpeg" << "png" << "gif";

    foreach(const QString &file, files) {
        foreach(const QString &cover, cover_list) {
            foreach(const QString &ext, ext_list) {
                if(file.compare(QString("%1.%2").arg(cover, ext), Qt::CaseInsensitive) == 0) {
                    qDebug() << "Trying to load album art from" << folder.absoluteFilePath(file);
                    AVDecoder decoder;
                    if(decoder.open(folder.absoluteFilePath(file))) {
                        int width, height;
                        data = decoder.getThumbnail(width, height);
                        metadata->data.thumbnail.width = width;
                        metadata->data.thumbnail.height = height;
                    }
                    // only try with the first match
                    break;
                }
            }
        }
    }
#endif
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
#ifndef FFMPEG_ENABLED
    }

    Q_UNUSED(metadata);
#else
    } else if(MASK_SET(type, Photo)) {
        AVDecoder decoder;

        if(decoder.open(path)) {
            int width, height;
            data = decoder.getThumbnail(width, height);
            metadata->data.thumbnail.width = width;
            metadata->data.thumbnail.height = height;
        }
    } else if(MASK_SET(type, Music)) {
        if(MASK_SET(type, Folder)) {
            // TODO: try to load an album cover from one of the audio files.
            data = findFolderAlbumArt(path, metadata);
        } else {
            AVDecoder decoder;

            if(decoder.open(path)) {
                int width, height;
                data = decoder.getThumbnail(width, height);
                metadata->data.thumbnail.width = width;
                metadata->data.thumbnail.height = height;
            }
        }
    }  else if(MASK_SET(type, Video)) {
        AVDecoder decoder;
        if(decoder.open(path)) {
            int percentage = QSettings().value("videoThumbnailSeekPercentage", 30).toInt();
            decoder.seekVideo(percentage);

            int width, height;

            data = decoder.getThumbnail(width, height);
            metadata->data.thumbnail.width = width;
            metadata->data.thumbnail.height = height;
        }
    }
#endif
    return data;
}

QString readable_size(qint64 size, bool use_gib)
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

int getVitaProtocolVersion()
{
    QString protocol_mode = QSettings().value("protocolMode", "automatic").toString();
    qDebug() << "Using protocol mode: " << protocol_mode;

    int protocol;

    if(protocol_mode == "manual")
    {
        int index = QSettings().value("protocolIndex").toInt();

        switch(index)
        {
        case 0:
            protocol = VITAMTP_PROTOCOL_FW_3_30;
            break;
        case 1:
            protocol = VITAMTP_PROTOCOL_FW_3_10;
            break;
        case 2:
            protocol = VITAMTP_PROTOCOL_FW_3_00;
            break;
        case 3:
            protocol = VITAMTP_PROTOCOL_FW_2_60;
            break;
        case 4:
            protocol = VITAMTP_PROTOCOL_FW_2_10;
            break;
        case 5:
            protocol = VITAMTP_PROTOCOL_FW_2_00;
            break;
        case 6:
            protocol = VITAMTP_PROTOCOL_FW_1_80;
            break;
        case 7:
            protocol = VITAMTP_PROTOCOL_FW_1_60;
            break;
        case 8:
            protocol = VITAMTP_PROTOCOL_FW_1_50;
            break;
        case 9:
            protocol = VITAMTP_PROTOCOL_FW_1_00;
            break;
        default:
            protocol = VITAMTP_PROTOCOL_MAX_VERSION;
            break;
        }
    }
    else if(protocol_mode == "custom")
    {
        bool ok;
        protocol = QSettings().value("protocolVersion").toInt(&ok);

        if(!ok || protocol <= 0)
            protocol = VITAMTP_PROTOCOL_MAX_VERSION;
    }
    else
        protocol = VITAMTP_PROTOCOL_MAX_VERSION;

    return protocol;
}
