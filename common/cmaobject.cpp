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
#include "database.h"
#include "cmautils.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QSettings>

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
    bool skipMetadata = QSettings().value("skipMetadata", false).toBool();
    SfoReader reader;

    if(!skipMetadata && reader.load(sfo)) {
        metadata.data.saveData.title = strdup(reader.value("TITLE", ""));
        //FIXME: disable savedata detail for now
        //QString detail(reader.value("SAVEDATA_DETAIL", ""));

        // libxml follow the spec and normalizes the newlines (and others) but
        // the PS Vita chokes on contiguous codes and crashes the CMA app on
        // the device. Of course, the "fix" would be that libxml doesn't
        // normalize newlines but that is aganist the XML spec that the PS Vita
        // doesn't respect >_>

        // convert DOS to UNIX newlines
        //detail.replace("\r\n", "\n");
        // separate newlines from quotes
        //detail.replace("\n\"", "\n \"");
        //detail.replace("\"\n", "\" \n");
        // merge consecutive newlines
        //detail.replace("\n\n", "\n");
        //while(detail.endsWith('\n')) {
        //    detail.chop(1);
        //}
        //metadata.data.saveData.detail = strdup(detail.toStdString().c_str());
        metadata.data.saveData.detail = strdup("");

        // remove newlines from savedata title
        QString title(reader.value("SAVEDATA_TITLE", ""));
        while(title.endsWith('\n')) {
            title.chop(1);
        }
        metadata.data.saveData.savedataTitle = strdup(title.toStdString().c_str());
        metadata.data.saveData.dateTimeUpdated = QFileInfo(sfo).created().toUTC().toTime_t();
    } else {
        metadata.data.saveData.title = strdup(metadata.name);
        metadata.data.saveData.detail = strdup("");
        metadata.data.saveData.savedataTitle = strdup("");
        metadata.data.saveData.dateTimeUpdated = 0;
    }
}

void CMAObject::initObject(const QFileInfo &file, int obj_file_type)
{
    metadata.name = strdup(file.fileName().toUtf8().data());
    metadata.ohfiParent = parent->metadata.ohfi;
    metadata.ohfi = ohfi_count++;

    metadata.type = VITA_DIR_TYPE_MASK_REGULAR; // ignored for files
    metadata.dateTimeCreated = file.created().toUTC().toTime_t();
    metadata.size = 0;
    DataType type = file.isFile() ? File : Folder;
    metadata.dataType = (DataType)(type | (parent->metadata.dataType & ~Folder));

    // create additional metadata
    if(MASK_SET(metadata.dataType, SaveData | Folder)) {
        metadata.data.saveData.dirName = strdup(metadata.name);
        metadata.data.saveData.statusType = 1;
        loadSfoMetadata(file.absoluteFilePath());
    } else if(MASK_SET(metadata.dataType, Music | File)) {

        if(obj_file_type < 0) {
            qWarning("Invalid file type for music: %i, setting it to zero", obj_file_type);
            obj_file_type = 0;
        }

        metadata.data.music.fileName = strdup(metadata.name);
        metadata.data.music.fileFormatType = audio_list[obj_file_type].file_format;
        metadata.data.music.statusType = 1;
        metadata.data.music.numTracks = 1;
        metadata.data.music.tracks = new media_track();
        metadata.data.music.tracks->type = VITA_TRACK_TYPE_AUDIO;
        metadata.data.music.tracks->data.track_photo.codecType = audio_list[obj_file_type].file_codec;
        Database::loadMusicMetadata(file.absoluteFilePath(), metadata);
    } else if(MASK_SET(metadata.dataType, Video | File)) {
        metadata.data.video.fileName = strdup(metadata.name);
        metadata.data.video.dateTimeUpdated = file.created().toUTC().toTime_t();
        metadata.data.video.statusType = 1;
        metadata.data.video.fileFormatType = FILE_FORMAT_MP4;
        metadata.data.video.parentalLevel = 0;
        metadata.data.video.numTracks = 1;
        metadata.data.video.tracks = new media_track();
        metadata.data.video.tracks->type = VITA_TRACK_TYPE_VIDEO;
        Database::loadVideoMetadata(file.absoluteFilePath(), metadata);
    } else if(MASK_SET(metadata.dataType, Photo | File)) {

        if(obj_file_type < 0) {
            qWarning("Invalid file type for photos: %i, setting it to zero", obj_file_type);
            obj_file_type = 0;
        }

        metadata.data.photo.fileName = strdup(metadata.name);
        metadata.data.photo.fileFormatType = photo_list[obj_file_type].file_format;
        metadata.data.photo.statusType = 1;
        metadata.data.photo.dateTimeOriginal = file.created().toUTC().toTime_t();
        metadata.data.photo.numTracks = 1;
        metadata.data.photo.tracks = new media_track();
        metadata.data.photo.tracks->type = VITA_TRACK_TYPE_PHOTO;
        metadata.data.photo.tracks->data.track_photo.codecType = photo_list[obj_file_type].file_codec;
        Database::loadPhotoMetadata(file.absoluteFilePath(), metadata);
    }

    m_path = file.absoluteFilePath();

    if(parent->metadata.path == NULL) {
        metadata.path = strdup(metadata.name);
    } else {
        QString newpath = QString(parent->metadata.path) + "/" + metadata.name;
        metadata.path = strdup(newpath.toUtf8().data());
    }

    updateObjectSize(file.size());
}

void CMAObject::updateObjectSize(qint64 size)
{
    if(parent) {
        parent->updateObjectSize(size);
    }
    metadata.size += size;
}

void CMAObject::rename(const QString &newname)
{
    free(metadata.name);
    metadata.name = strdup(newname.toUtf8().data());

    if(metadata.path) {
        QStringList metadata_path(QString(metadata.path).split("/"));
        metadata_path.replace(metadata_path.count() - 1, newname);
        free(metadata.path);
        metadata.path = strdup(metadata_path.join("/").toUtf8().data());
    }

    m_path = QFileInfo(m_path).absoluteDir().path() + "/" + newname;
}

void CMAObject::refreshPath()
{
    if(parent) {
        free(metadata.path);
        QString newpath(QString(parent->metadata.path) + "/" + metadata.name);
        metadata.path = strdup(newpath.toUtf8().data());
        m_path = parent->m_path + "/" + metadata.name;
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
