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

#include "cmaobject.h"
#include "sforeader.h"
#include "utils.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>

#include <MediaInfo/MediaInfo.h>
#include <MediaInfo/File__Analyse_Automatic.h>

int CMAObject::ohfi_count = OHFI_OFFSET;

CMAObject::CMAObject(CMAObject *obj_parent) :
    parent(obj_parent), metadata()
{
}

CMAObject::~CMAObject()
{
    free(metadata.name);
    free(metadata.path);

    if(MASK_SET(metadata.dataType, SaveData | Folder)) {
        free(metadata.data.saveData.title);
        free(metadata.data.saveData.detail);
        free(metadata.data.saveData.dirName);
        free(metadata.data.saveData.savedataTitle);
    } else if(MASK_SET(metadata.dataType, Photo | File)) {
        free(metadata.data.photo.title);
        free(metadata.data.photo.fileName);
        delete metadata.data.photo.tracks;
    } else if(MASK_SET(metadata.dataType, Music | File)) {
        free(metadata.data.music.title);
        free(metadata.data.music.fileName);
        free(metadata.data.music.album);
        free(metadata.data.music.artist);
        delete metadata.data.music.tracks;
    } else if(MASK_SET(metadata.dataType, Video | File)) {
        free(metadata.data.video.title);
        free(metadata.data.video.explanation);
        free(metadata.data.video.fileName);
        free(metadata.data.video.copyright);
        delete metadata.data.video.tracks;
    }
}

void CMAObject::loadSfoMetadata(const QString &path)
{
    QString sfo = QDir(path).absoluteFilePath("PARAM.SFO");
    SfoReader reader;

    if(reader.load(sfo)) {
        metadata.data.saveData.title = strdup(reader.value("TITLE", ""));
        metadata.data.saveData.detail = strdup(reader.value("SAVEDATA_DETAIL", ""));
        metadata.data.saveData.savedataTitle = strdup(reader.value("SAVEDATA_TITLE", ""));
        metadata.data.saveData.dateTimeUpdated = QFileInfo(sfo).created().toTime_t();
    } else {
        metadata.data.saveData.title = strdup(metadata.name);
        metadata.data.saveData.detail = strdup("");
        metadata.data.saveData.savedataTitle = strdup("");
        metadata.data.saveData.dateTimeUpdated = 0;
    }
}

void CMAObject::loadMusicMetadata(const QString &path)
{
    MediaInfoLib::MediaInfo media;
    if(media.Open(path.toStdWString())) {
        QString data;
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Album));
        metadata.data.music.album = strdup(!data.isEmpty() ? data.toUtf8().data() : (parent->metadata.name ? parent->metadata.name : ""));
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Performer));
        metadata.data.music.artist = strdup(data.toUtf8().data());
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Title));
        metadata.data.music.title = strdup(!data.isEmpty() ? data.toUtf8().data() : metadata.name);
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Audio, 0, MediaInfoLib::Audio_BitRate));
        bool ok;
        int bitrate = data.toInt(&ok);
        if(ok) {
            metadata.data.music.tracks->data.track_audio.bitrate = bitrate;
        }
    } else {
        metadata.data.music.album = strdup(parent->metadata.name ? parent->metadata.name : "");
        metadata.data.music.artist = strdup("");
        metadata.data.music.title = strdup(metadata.name);
    }
}

void CMAObject::loadVideoMetadata(const QString &path)
{
    MediaInfoLib::MediaInfo media;
    if(media.Open(path.toStdWString())) {
        bool ok;
        QString data;
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Title));
        metadata.data.video.title = strdup(!data.isEmpty() ? data.toUtf8().data() : metadata.name);
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Comment));
        metadata.data.video.explanation = strdup(data.toUtf8().data());
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Copyright));
        metadata.data.video.copyright = strdup(data.toUtf8().data());
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Video, 0, MediaInfoLib::Video_BitRate));
        int bitrate = data.toInt(&ok);
        if(ok) {
            metadata.data.video.tracks->data.track_video.bitrate = bitrate;
        }
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Video, 0, MediaInfoLib::Video_Duration));
        int duration = data.toInt(&ok);
        if(ok) {
            metadata.data.video.tracks->data.track_video.duration = duration;
        }
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Video, 0, MediaInfoLib::Video_Width));
        int width = data.toInt(&ok);
        if(ok) {
            metadata.data.video.tracks->data.track_video.width = width;
        }
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Video, 0, MediaInfoLib::Video_Height));
        int height = data.toInt(&ok);
        if(ok) {
            metadata.data.video.tracks->data.track_video.height = height;
        }
    } else {
        metadata.data.video.title = strdup(metadata.name);
        metadata.data.video.explanation = strdup("");
        metadata.data.video.copyright = strdup("");
    }
}

void CMAObject::loadPhotoMetadata(const QString &path)
{
    MediaInfoLib::MediaInfo media;
    if(media.Open(path.toStdWString())) {
        bool ok;
        QString data;
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_General, 0, MediaInfoLib::General_Title));
        metadata.data.photo.title = strdup(!data.isEmpty() ? data.toUtf8().data() : metadata.name);
        data = QString::fromStdWString(media.Get(MediaInfoLib::Stream_Image, 0, MediaInfoLib::Image_Height));
        int height = data.toInt(&ok);
        if(ok) {
            metadata.data.photo.tracks->data.track_photo.height = height;
        }
        int width = data.toInt(&ok);
        if(ok) {
            metadata.data.photo.tracks->data.track_photo.width = width;
        }
    } else {
        metadata.data.photo.title = strdup(metadata.name);
    }
}

void CMAObject::initObject(const QFileInfo &file)
{
    metadata.name = strdup(file.fileName().toUtf8().data());
    metadata.ohfiParent = parent->metadata.ohfi;
    metadata.ohfi = ohfi_count++;

    metadata.type = VITA_DIR_TYPE_MASK_REGULAR; // ignored for files
    metadata.dateTimeCreated = file.created().toTime_t();
    metadata.size = file.size();
    DataType type = file.isFile() ? File : Folder;
    metadata.dataType = (DataType)(type | (parent->metadata.dataType & ~Folder));

    // create additional metadata
    if(MASK_SET(metadata.dataType, SaveData | Folder)) {
        metadata.data.saveData.dirName = strdup(metadata.name);
        metadata.data.saveData.statusType = 1;        
        loadSfoMetadata(file.absoluteFilePath());
    } else if(MASK_SET(metadata.dataType, Music | File)) {
        metadata.data.music.fileName = strdup(metadata.name);
        metadata.data.music.fileFormatType = 20;
        metadata.data.music.statusType = 1;
        metadata.data.music.numTracks = 1;
        metadata.data.music.tracks = new media_track();
        metadata.data.music.tracks->type = VITA_TRACK_TYPE_AUDIO;
        metadata.data.music.tracks->data.track_photo.codecType = 12; // MP3?
        loadMusicMetadata(file.absoluteFilePath());
    } else if(MASK_SET(metadata.dataType, Video | File)) {
        metadata.data.video.fileName = strdup(metadata.name);
        metadata.data.video.dateTimeUpdated = file.created().toTime_t();
        metadata.data.video.statusType = 1;
        metadata.data.video.fileFormatType = 1;
        metadata.data.video.parentalLevel = 0;
        metadata.data.video.numTracks = 1;
        metadata.data.video.tracks = new media_track();
        metadata.data.video.tracks->type = VITA_TRACK_TYPE_VIDEO;
        metadata.data.video.tracks->data.track_video.codecType = 3; // this codec is working
        loadVideoMetadata(file.absoluteFilePath());
    } else if(MASK_SET(metadata.dataType, Photo | File)) {
        metadata.data.photo.fileName = strdup(metadata.name);
        metadata.data.photo.fileFormatType = 28; // working
        metadata.data.photo.statusType = 1;
        metadata.data.photo.dateTimeOriginal = file.created().toTime_t();
        metadata.data.photo.numTracks = 1;
        metadata.data.photo.tracks = new media_track();
        metadata.data.photo.tracks->type = VITA_TRACK_TYPE_PHOTO;
        metadata.data.photo.tracks->data.track_photo.codecType = 17; // JPEG?
        loadPhotoMetadata(file.absoluteFilePath());
    }

    path = file.absoluteFilePath();

    if(parent->metadata.path == NULL) {
        metadata.path = strdup(metadata.name);
    } else {
        QString newpath = QString(parent->metadata.path) + QDir::separator() + metadata.name;
        metadata.path = strdup(newpath.toUtf8().data());
    }

    updateParentSize(metadata.size);
}

bool CMAObject::removeReferencedObject()
{
    if(metadata.dataType & Folder) {
        return removeRecursively(path);
    } else {
        return QFile::remove(path);
    }
}

void CMAObject::updateParentSize(unsigned long size)
{
    if(parent) {
        parent->metadata.size += size;
        parent->updateParentSize(size);
    }
}

void CMAObject::rename(const QString &newname)
{
    free(metadata.name);
    metadata.name = strdup(newname.toUtf8().data());

    if(metadata.path) {
        QStringList metadata_path(QString(metadata.path).split(QDir::separator()));
        metadata_path.replace(metadata_path.count() - 1, newname);
        free(metadata.path);
        metadata.path = strdup(metadata_path.join(QDir::separator()).toUtf8().data());
    }

    path = QFileInfo(path).absoluteDir().path() + QDir::separator() + newname;
}

void CMAObject::refreshPath()
{
    if(parent) {
        free(metadata.path);
        QString newpath(QString(parent->metadata.path) + QDir::separator() + metadata.name);
        metadata.path = strdup(newpath.toUtf8().data());
        path = parent->path + QDir::separator() + metadata.name;
    }
}

bool CMAObject::hasParent(const CMAObject *obj)
{
    if(parent) {
        if(metadata.ohfiParent == obj->metadata.ohfi) {
            return true;
        } else {
            return parent->hasParent(obj);
        }
    }
    return false;
}

bool CMAObject::operator==(const CMAObject &obj)
{
    return metadata.ohfi == obj.metadata.ohfi;
}

bool CMAObject::operator!=(const CMAObject &obj)
{
    return metadata.ohfi != obj.metadata.ohfi;
}

bool CMAObject::operator<(const CMAObject &obj)
{
    return metadata.ohfi < obj.metadata.ohfi;
}
