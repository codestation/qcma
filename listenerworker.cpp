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

#include "listenerworker.h"

#include <locale.h>

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QUrl>
#include <QMutexLocker>
#include <QDebug>

#include "utils.h"

#define CALL_MEMBER_FUNC(obj,memberptr)  ((obj).*(memberptr))

const metadata_t ListenerWorker::g_thumbmeta = {0, 0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {{18, 144, 80, 0, 1, 1.0f, 2}}, NULL};

ListenerWorker::ListenerWorker(QObject *parent) :
    BaseWorker(parent)
{
    connected = true;
}

int ListenerWorker::rebuildDatabase()
{
    db.destroy();
    return db.create();
}

void ListenerWorker::setDevice(vita_device_t *device)
{
    this->device = device;
}

bool ListenerWorker::isConnected()
{
    return connected;
}

void ListenerWorker::disconnect()
{
    qDebug("Stopping event listener");
    connected = false;
}

void ListenerWorker::process()
{
    vita_event_t event;

    qDebug() << "From listener: "<< QThread::currentThreadId();

    while(connected) {
        if(VitaMTP_Read_Event(device, &event) < 0) {
            qWarning("Error reading event from Vita.");
            connected = false;
            break;
        }

        qDebug("Event 0x%04X recieved", event.Code);

        switch(event.Code) {
        case PTP_EC_VITA_RequestSendNumOfObject:
            vitaEventSendNumOfObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendObjectMetadata:
            vitaEventSendObjectMetadata(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendObject:
            vitaEventSendObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestCancelTask: // unimplemented
            vitaEventCancelTask(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendHttpObjectFromURL:
            vitaEventSendHttpObjectFromURL(&event, event.Param1);
            break;
        case PTP_EC_VITA_Unknown1: // unimplemented
            vitaEventUnimplementated(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendObjectStatus:
            vitaEventSendObjectStatus(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendObjectThumb:
            vitaEventSendObjectThumb(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestDeleteObject:
            vitaEventDeleteObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestGetSettingInfo:
            vitaEventGetSettingInfo(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendHttpObjectPropFromURL:
            vitaEventSendHttpObjectPropFromURL(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendPartOfObject:
            vitaEventSendPartOfObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestOperateObject:
            vitaEventOperateObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestGetPartOfObject:
            vitaEventGetPartOfObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendStorageSize:
            vitaEventSendStorageSize(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestCheckExistance:
            vitaEventCheckExistance(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestGetTreatObject:
            vitaEventGetTreatObject(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendCopyConfirmationInfo:
            vitaEventSendCopyConfirmationInfo(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendObjectMetadataItems:
            vitaEventSendObjectMetadataItems(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestSendNPAccountInfo:
            vitaEventSendNPAccountInfo(&event, event.Param1);
            break;
        case PTP_EC_VITA_RequestTerminate:
            vitaEventRequestTerminate(&event, event.Param1);
            break;
        default:
            vitaEventUnimplementated(&event, event.Param1);
        }
    }
    emit finished();
}

quint16 ListenerWorker::processAllObjects(CMAObject *parent, quint32 handle)
{
    union {
        unsigned char *fileData;
        uint32_t *handles;
    } data;

    metadata_t remote_meta;
    unsigned int length;

    if(VitaMTP_GetObject(device, handle, &remote_meta, (void **)&data, &length) != PTP_RC_OK) {
        qWarning("Cannot get object for handle %d", handle);
        return PTP_RC_VITA_Invalid_Data;
    }


    CMAObject *object =  db.pathToObject(remote_meta.name, parent->metadata.ohfi);

    if(object) {
        qDebug("Deleting %s", object->path.toStdString().c_str());
        removeRecursively(object->path);
        db.remove(object);
    }

    QDir dir(parent->path);

    if(remote_meta.dataType & Folder) {
        if(!dir.mkpath(remote_meta.name)) {
            qWarning("Cannot create directory: %s", remote_meta.name);
            free(data.fileData);
            free(remote_meta.name);
            return PTP_RC_VITA_Failed_Operate_Object;
        }
    } else {
        QFile file(dir.absoluteFilePath(remote_meta.name));

        if(!file.open(QIODevice::WriteOnly)) {
            qWarning("Cannot write to %s", remote_meta.name);
            free(data.fileData);
            free(remote_meta.name);
            return PTP_RC_VITA_Invalid_Permission;
        } else {
            file.write((const char *)data.fileData, remote_meta.size);
        }
    }

    QFileInfo info(dir, remote_meta.name);
    object = new CMAObject(parent);
    object->initObject(info);
    object->metadata.handle = remote_meta.handle;
    db.append(parent->metadata.ohfi, object);
    free(remote_meta.name);

    qDebug("Added object %s with OHFI %i to database", object->metadata.path, object->metadata.ohfi);

    if(remote_meta.dataType & Folder) {
        for(unsigned int i = 0; i < length; i++) {
            quint16 ret = processAllObjects(object, data.handles[i]);

            if(ret != PTP_RC_OK) {
                qDebug("Deleteting object with OHFI %d", object->metadata.ohfi);
                db.remove(object);
                free(data.fileData);
                return ret;
            }
        }
    }

    free(data.fileData);
    return PTP_RC_OK;
}

void ListenerWorker::vitaEventGetTreatObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    treat_object_t treatObject;

    if(VitaMTP_GetTreatObject(device, eventId, &treatObject) != PTP_RC_OK) {
        qWarning("Cannot get information on object to get");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *parent = db.ohfiToObject(treatObject.ohfiParent);

    if(parent == NULL) {
        qWarning("Cannot find parent OHFI %d", treatObject.ohfiParent);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    VitaMTP_ReportResult(device, eventId, processAllObjects(parent, treatObject.handle));
}

void ListenerWorker::vitaEventSendCopyConfirmationInfo(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    copy_confirmation_info_t *info;
    if(VitaMTP_SendCopyConfirmationInfoInit(device, eventId, &info) != PTP_RC_OK) {
        qWarning("Error recieving initial information.");
        return;
    }

    QMutexLocker locker(&db.mutex);

    quint64 size = 0;

    for(quint32 i = 0; i < info->count; i++) {
        CMAObject *object;

        if((object = db.ohfiToObject(info->ohfi[i])) == NULL) {
            qWarning("Cannot find OHFI %d", info->ohfi[i]);
            free(info);
            return;
        }

        size += object->metadata.size;
    }

    if(VitaMTP_SendCopyConfirmationInfo(device, eventId, info, size) != PTP_RC_OK) {
        qWarning("Error sending copy confirmation");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(info);
}

void ListenerWorker::vitaEventSendObjectMetadataItems(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    quint32 ohfi;
    if(VitaMTP_SendObjectMetadataItems(device, eventId, &ohfi) != PTP_RC_OK) {
        qWarning("Cannot get OHFI for retreving metadata");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *object = db.ohfiToObject(ohfi);

    if(object == NULL) {
        qWarning("Cannot find OHFI %d in database", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    metadata_t *metadata = &object->metadata;
    metadata->next_metadata = NULL;
    qDebug("Sending metadata for OHFI %d (%s)", ohfi, metadata->path);

    if(VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK) {
        qWarning("Error sending metadata");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void ListenerWorker::vitaEventSendNPAccountInfo(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    qWarning("Event 0x%x unimplemented!", event->Code);
}

void ListenerWorker::vitaEventRequestTerminate(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);
    qWarning("Event 0x%x unimplemented!", event->Code);
}

void ListenerWorker::vitaEventUnimplementated(vita_event_t *event, int eventId)
{
    qWarning("Unknown event not handled, code: 0x%x, id: %d", event->Code, eventId);
    qWarning("Param1: 0x%08X, Param2: 0x%08X, Param3: 0x%08X", event->Param1, event->Param2, event->Param3);
}

void ListenerWorker::vitaEventSendNumOfObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    QMutexLocker locker(&db.mutex);

    uint ohfi = event->Param2;
    int items = db.filterObjects(ohfi, NULL);

    if(VitaMTP_SendNumOfObject(device, eventId, items) != PTP_RC_OK) {
        qWarning("Error occured receiving object count for OHFI parent %d", ohfi);
    } else {
        qDebug("Returned count of %d objects for OHFI parent %d", items, ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void ListenerWorker::vitaEventSendObjectMetadata(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    browse_info_t browse;

    if(VitaMTP_GetBrowseInfo(device, eventId, &browse) != PTP_RC_OK) {
        qWarning("GetBrowseInfo failed");
        return;
    }
    QMutexLocker locker(&db.mutex);

    metadata_t *meta;
    int count = db.filterObjects(browse.ohfiParent, &meta);  // if meta is null, will return empty XML
    qDebug("Sending %i metadata filtered objects for OHFI %id", count, browse.ohfiParent);

    if(VitaMTP_SendObjectMetadata(device, eventId, meta) != PTP_RC_OK) {  // send all objects with OHFI parent
        qWarning("Sending metadata for OHFI parent %d failed", browse.ohfiParent);
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void ListenerWorker::vitaEventSendObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    int ohfi = event->Param2;

    QMutexLocker locker(&db.mutex);

    Database::find_data iters;
    if(!db.find(ohfi, iters)) {
        qWarning("Failed to find OHFI %d", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    unsigned long len = 0;
    CMAObject *object = *iters.it;
    CMAObject *start = object;
    uint parentHandle = event->Param3;
    uint handle;

    do {
        uchar *data = NULL;
        len = object->metadata.size;
        QFile file(object->path);

        // read the file to send if it's not a directory
        // if it is a directory, data and len are not used by VitaMTP
        if(object->metadata.dataType & File) {
            if(!file.open(QIODevice::ReadOnly) || (data = file.map(0, file.size())) == NULL) {
                qWarning("Failed to read %s", object->path.toStdString().c_str());
                VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
                return;
            }
        }

        // get the PTP object ID for the parent to put the object
        // we know the parent has to be before the current node
        // the first time this is called, parentHandle is left untouched

        if(start != object) {
            parentHandle = object->parent->metadata.handle;
        }

        // send the data over
        qDebug("Sending %s of %lu bytes to device", object->metadata.name, len);
        qDebug("OHFI %d with handle 0x%08X", ohfi, parentHandle);

        if(VitaMTP_SendObject(device, &parentHandle, &handle, &object->metadata, data) != PTP_RC_OK) {
            qWarning("Sending of %s failed.", object->metadata.name);
            file.unmap(data);
            return;
        }

        object->metadata.handle = handle;
        file.unmap(data);
        object = *iters.it++;

    } while(iters.it != iters.end && object->metadata.ohfiParent >= OHFI_OFFSET); // get everything under this "folder"

    VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, handle);
    VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);  // TODO: Send thumbnail
}

void ListenerWorker::vitaEventCancelTask(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    int eventIdToCancel = event->Param2;
    VitaMTP_CancelTask(device, eventIdToCancel);
    qWarning("Event CancelTask (0x%x) unimplemented!", event->Code);
}

void ListenerWorker::vitaEventSendHttpObjectFromURL(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    char *url;
    if(VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK) {
        qWarning("Failed to recieve URL");
        return;
    }

    QString urlpath = QSettings().value("urlPath").toString();
    QString basename = QFileInfo(QUrl(url).path()).fileName();
    QFile file(QDir(urlpath).absoluteFilePath(basename));

    QByteArray data;

    if(!file.open(QIODevice::ReadOnly)) {
        if(basename == "psp2-updatelist.xml") {
            qDebug("Found request for update list. Sending cached data");
            QFile res(":/main/psp2-updatelist.xml");
            res.open(QIODevice::ReadOnly);
            data = res.readAll();
        } else {
            qWarning("Failed to download %s", url);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Download);
            free(url);
            return;
        }
    } else {
        data = file.readAll();
    }

    qDebug("Sending %i bytes of data for HTTP request %s", data.size(), url);

    if(VitaMTP_SendHttpObjectFromURL(device, eventId, data.data(), data.size()) != PTP_RC_OK) {
        qWarning("Failed to send HTTP object");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(url);
}

void ListenerWorker::vitaEventSendObjectStatus(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    object_status_t objectstatus;

    if(VitaMTP_SendObjectStatus(device, eventId, &objectstatus) != PTP_RC_OK) {
        qWarning("Failed to get information for object status.");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *object = db.pathToObject(objectstatus.title, objectstatus.ohfiRoot);

    if(object == NULL) { // not in database, don't return metadata
        qDebug("Object %s not in database (OHFI: %i). Sending OK response for non-existence", objectstatus.title, objectstatus.ohfiRoot);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    } else {
        metadata_t *metadata = &object->metadata;
        metadata->next_metadata = NULL;
        qDebug("Sending metadata for OHFI %d", object->metadata.ohfi);

        if(VitaMTP_SendObjectMetadata(device, eventId, metadata) != PTP_RC_OK) {
            qWarning("Error sending metadata for %d", object->metadata.ohfi);
        } else {
            VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
        }
    }

    free(objectstatus.title);
}

void ListenerWorker::vitaEventSendObjectThumb(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    QMutexLocker locker(&db.mutex);

    int ohfi = event->Param2;
    CMAObject *object = db.ohfiToObject(ohfi);

    if(object == NULL) {
        qWarning("Cannot find OHFI %d in database.", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    QByteArray data;

    if(MASK_SET(object->metadata.dataType, SaveData)) {
        QString thumbpath = QDir(object->path).absoluteFilePath("ICON0.PNG");
        qDebug("Sending savedata thumbnail from %s", thumbpath.toStdString().c_str());

        QFile file(thumbpath);
        if(!file.open(QIODevice::ReadOnly)) {
            qWarning("Cannot find thumbnail %s", thumbpath.toStdString().c_str());
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);
            return;
        }
        data = file.readAll();

    } else {
        QImage img;
        if(!MASK_SET(object->metadata.dataType, Photo) || !img.load(object->path)) {
            qWarning("Thumbnail sending for the file %s is not supported", object->metadata.path);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Data);
            return;
        }
        qDebug("Creating thumbnail of %s", object->metadata.name);

        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QImage result = img.scaled(256, 256, Qt::KeepAspectRatio, Qt::FastTransformation);
        result.save(&buffer, "JPEG");
    }

    // workaround for the vitamtp locale bug
    char *locale = strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "C");

    if(VitaMTP_SendObjectThumb(device, eventId, (metadata_t *)&g_thumbmeta, (uchar *)data.data(), data.size()) != PTP_RC_OK) {
        qWarning("Error sending thumbnail");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    // restore locale
    setlocale(LC_ALL, locale);
    free(locale);
}

void ListenerWorker::vitaEventDeleteObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    QMutexLocker locker(&db.mutex);

    int ohfi = event->Param2;
    CMAObject *object = db.ohfiToObject(ohfi);

    if(object == NULL) {
        qWarning("OHFI %d not found", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    qDebug("Deleting %s, OHFI: %i", object->metadata.path, object->metadata.ohfi);
    removeRecursively(object->path);    
    db.remove(object);

    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void ListenerWorker::vitaEventGetSettingInfo(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    settings_info_t *settingsinfo;
    if(VitaMTP_GetSettingInfo(device, eventId, &settingsinfo) != PTP_RC_OK) {
        qWarning("Failed to get setting info from Vita.");
        return;
    }

    qDebug("Current account id: %s", settingsinfo->current_account.accountId);

    db.setUUID(settingsinfo->current_account.accountId);

    // set the database to be updated ASAP    
    emit refreshDatabase();

    // free all the information
    VitaMTP_Data_Free_Settings(settingsinfo);
    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}

void ListenerWorker::vitaEventSendHttpObjectPropFromURL(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    char *url;
    if(VitaMTP_GetUrl(device, eventId, &url) != PTP_RC_OK) {
        qWarning("Failed to get URL");
        return;
    }

    QString urlpath = QSettings().value("urlPath").toString();
    QString basename = QFileInfo(url).fileName();
    QFileInfo file(QDir(urlpath).absoluteFilePath(basename));

    if(!file.exists()) {
        qWarning("The file %s is not accesible", url);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Download);
        free(url);
        return;
    }

    QString timestamp = file.lastModified().toString();

    http_object_prop_t httpobjectprop;
    httpobjectprop.timestamp = timestamp.toUtf8().data();
    httpobjectprop.timestamp_len = timestamp.toUtf8().size();

    if(VitaMTP_SendHttpObjectPropFromURL(device, eventId, &httpobjectprop) != PTP_RC_OK) {
        qWarning("Failed to send object properties");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(url);
}

void ListenerWorker::vitaEventSendPartOfObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    send_part_init_t part_init;

    if(VitaMTP_SendPartOfObjectInit(device, eventId, &part_init) != PTP_RC_OK) {
        qWarning("Cannot get information on object to send");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *object = db.ohfiToObject(part_init.ohfi);

    if(object == NULL) {
        qWarning("Cannot find object for OHFI %d", part_init.ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }

    QFile file(object->path);

    if(!file.open(QIODevice::ReadOnly)) {
        qWarning("Cannot read %s", object->path.toStdString().c_str());
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    } else {
        file.seek(part_init.offset);
        QByteArray data = file.read(part_init.size);
        qDebug("Sending %s at file offset %zu for %zu bytes", object->metadata.path, part_init.offset, part_init.size);

        if(VitaMTP_SendPartOfObject(device, eventId, (unsigned char *)data.data(), data.size()) != PTP_RC_OK) {
            qWarning("Failed to send part of object OHFI %d", part_init.ohfi);
        } else {
            VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
        }
    }
}

void ListenerWorker::vitaEventOperateObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    operate_object_t operateobject;

    if(VitaMTP_OperateObject(device, eventId, &operateobject) != PTP_RC_OK) {
        qWarning("Cannot get information on object to operate");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *root = db.ohfiToObject(operateobject.ohfi);

    // end for renaming only
    if(root == NULL) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }

    switch(operateobject.cmd) {
    case VITA_OPERATE_CREATE_FOLDER: {
        qDebug("Operate command %d: Create folder %s", operateobject.cmd, operateobject.title);

        QDir dir(root->path);
        if(!dir.mkdir(operateobject.title)) {
            qWarning("Unable to create temporary folder: %s", operateobject.title);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        } else {
            CMAObject *newobj = new CMAObject(root);
            newobj->initObject(QFileInfo(dir, operateobject.title));
            db.append(operateobject.ohfi, newobj);
            qDebug("Created folder %s with OHFI %d under parent %s", newobj->metadata.path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
        }
        break;
    }
    case VITA_OPERATE_CREATE_FILE: {
        qDebug("Operate command %d: Create file %s", operateobject.cmd, operateobject.title);

        QFile file(root->path + QDir::separator() + operateobject.title);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("Unable to create temporary file: %s", operateobject.title);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        } else {
            CMAObject *newobj = new CMAObject(root);
            newobj->initObject(file);
            db.append(root->metadata.ohfi, newobj);
            qDebug("Created file %s with OHFI %d under parent %s", newobj->metadata.path, newobj->metadata.ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, newobj->metadata.ohfi);
        }
        break;
    }
    case VITA_OPERATE_RENAME: {
        qDebug("Operate command %d: Rename %s to %s", operateobject.cmd, root->metadata.name, operateobject.title);

        QString oldpath = root->path;
        QString oldname = root->metadata.name;

        //rename the current object
        root->rename(operateobject.title);
        Database::find_data iters;
        db.find(root->metadata.ohfi, iters);

        // rename the rest of the list only if has the renamed parent in some part of the chain
        while(iters.it != iters.end) {
            CMAObject *obj = *iters.it++;

            if(obj->hasParent(root)) {
                obj->refreshPath();
            }
        }

        // rename in filesystem
        if(!QFile(oldpath).rename(root->path)) {
            qWarning("Unable to rename %s to %s", oldname.toStdString().c_str(), operateobject.title);
            VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
        }

        qDebug("Renamed OHFI %d from %s to %s", root->metadata.ohfi, oldname.toStdString().c_str(), root->metadata.name);
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_OK, root->metadata.ohfi);
        break;
    }

    default:
        qWarning("Operate command %d: Not implemented", operateobject.cmd);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        break;
    }

    free(operateobject.title);
}

void ListenerWorker::vitaEventGetPartOfObject(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    unsigned char *data;
    send_part_init_t part_init;

    if(VitaMTP_GetPartOfObject(device, eventId, &part_init, &data) != PTP_RC_OK) {
        qWarning("Cannot get object from device");
        return;
    }

    QMutexLocker locker(&db.mutex);
    CMAObject *object = db.ohfiToObject(part_init.ohfi);

    if(object == NULL) {
        qWarning("Cannot find OHFI %d", part_init.ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        free(data);
        return;
    }

    qDebug("Receiving %s at offset %zu for %zu bytes", object->metadata.path, part_init.offset, part_init.size);

    QFile file(object->path);
    if(!file.open(QIODevice::ReadWrite)) {
        qWarning("Cannot write to file %s", object->path.toStdString().c_str());
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
    } else {
        file.seek(part_init.offset);
        file.write((const char *)data, part_init.size);
        object->metadata.size += part_init.size;
        object->updateParentSize(part_init.size);
        qDebug("Written %zu bytes to %s at offset %zu.", part_init.size, object->path.toStdString().c_str(), part_init.offset);
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }

    free(data);
}

void ListenerWorker::vitaEventSendStorageSize(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    QMutexLocker locker(&db.mutex);

    int ohfi = event->Param2;
    CMAObject *object = db.ohfiToObject(ohfi);

    if(object == NULL) {
        qWarning("Error: Cannot find OHFI %d", ohfi);
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    } else {
        QFile file(object->path);

        if(!file.exists()) {
            // create the directory if doesn't exist so the query don't fail
            qDebug("Creating %s", object->path.toStdString().c_str());

            if(!QDir(QDir::root()).mkpath(object->path)) {
                qWarning("Create directory failed");
                VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
                return;
            }
        }
    }

    quint64 total;
    quint64 free;

    if(!getDiskSpace(object->path, &free, &total)) {
        qWarning("Cannot get disk space");
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Invalid_Permission);
        return;
    }

    qDebug("Storage stats for drive containing OHFI %d, free: %llu, total: %llu", ohfi, free, total);

    if(VitaMTP_SendStorageSize(device, eventId, total, free) != PTP_RC_OK) {
        qWarning("Send storage size failed");
    } else {
        VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
    }
}

void ListenerWorker::vitaEventCheckExistance(vita_event_t *event, int eventId)
{
    qDebug("Event recieved in %s, code: 0x%x, id: %d", Q_FUNC_INFO, event->Code, eventId);

    int handle = event->Param2;
    existance_object_t existance;

    if(VitaMTP_CheckExistance(device, handle, &existance) != PTP_RC_OK) {
        qWarning("Cannot read information on object to be sent");
        return;
    }

    QMutexLocker locker(&db.mutex);

    CMAObject *object = db.pathToObject(existance.name, 0);

    if(object == NULL) {
        VitaMTP_ReportResult(device, eventId, PTP_RC_VITA_Different_Object);
    } else {
        VitaMTP_ReportResultWithParam(device, eventId, PTP_RC_VITA_Same_Object, object->metadata.ohfi);
    }

    VitaMTP_ReportResult(device, eventId, PTP_RC_OK);
}
