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

#include "cmaevent.h"
#include "cmautils.h"
#include <locale.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QThread>
#include <QUrl>

QFile *CmaEvent::m_file = NULL;

const QString hash360 = "8cc2e2666626c4ff8f582bf209473526e825e2a5e38e39b259a8a46e25ef371c";

static metadata_t g_thumbmeta = {0, 0, 0, NULL, NULL, 0, 0, 0, Thumbnail, {{17, 240, 136, 0, 1, 1.0f, 2}}, NULL};

CmaEvent::CmaEvent(Database *db, vita_device_t *s_device) :
    m_device(s_device), m_db(db), is_active(true)
{
}

void CmaEvent::process()
{
    qDebug("Starting event_thread: 0x%016" PRIxPTR, (uintptr_t)QThread::currentThreadId());

    while(true) {
        sema.acquire();
        if(!isActive()) {
            break;
        }

        QMutexLocker locker(&mutex);
        processEvent();
    }
    qDebug("Finishing event_thread");
    emit finishedEventLoop();
}

bool CmaEvent::isActive()
{
    QMutexLocker locker(&active);
    return is_active;
}

void CmaEvent::stop()
{
    QMutexLocker locker(&active);
    is_active = false;
    sema.release();
}

void CmaEvent::setDevice(vita_device_t *device)
{
    QMutexLocker locker(&mutex);
    m_device = device;
}

void CmaEvent::setEvent(vita_event_t cma_event)
{
    QMutexLocker locker(&mutex);
    m_event = cma_event;
    locker.unlock();
    sema.release();
}

void CmaEvent::processEvent()
{
    switch(m_event.Code) {
    case PTP_EC_VITA_RequestSendNumOfObject:
        vitaEventSendNumOfObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendObjectMetadata:
        vitaEventSendObjectMetadata(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendObject:
        vitaEventSendObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendHttpObjectFromURL:
        vitaEventSendHttpObjectFromURL(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_Unknown1: // unimplemented
        vitaEventUnimplementated(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendObjectStatus:
        vitaEventSendObjectStatus(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendObjectThumb:
        vitaEventSendObjectThumb(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestDeleteObject:
        vitaEventDeleteObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestGetSettingInfo:
        vitaEventGetSettingInfo(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendHttpObjectPropFromURL:
        vitaEventSendHttpObjectPropFromURL(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendPartOfObject:
        vitaEventSendPartOfObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestOperateObject:
        vitaEventOperateObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestGetPartOfObject:
        vitaEventGetPartOfObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendStorageSize:
        vitaEventSendStorageSize(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestCheckExistance:
        vitaEventCheckExistance(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestGetTreatObject:
        vitaEventGetTreatObject(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendCopyConfirmationInfo:
        vitaEventSendCopyConfirmationInfo(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendObjectMetadataItems:
        vitaEventSendObjectMetadataItems(&m_event, m_event.Param1);
        break;
    case PTP_EC_VITA_RequestSendNPAccountInfo:
        vitaEventSendNPAccountInfo(&m_event, m_event.Param1);
        break;
    default:
        vitaEventUnimplementated(&m_event, m_event.Param1);
    }
    qDebug("Ended event, code: 0x%x, id: %d", m_event.Code, m_event.Param1);
}

quint16 CmaEvent::processAllObjects(metadata_t &parent_metadata, quint32 handle)
{
    qDebug("Called %s, handle: %d, parent name: %s", Q_FUNC_INFO, handle, parent_metadata.name);

    char *name;
    int dataType;
    quint32 *p_handles;
    unsigned int p_len;

    if(VitaMTP_GetObject_Info(m_device, handle, &name, &dataType) != PTP_RC_OK) {
        qWarning("Cannot get object for handle %d", handle);
        return PTP_RC_VITA_Invalid_Data;
    }

    if(dataType & Folder) {
        if(VitaMTP_GetObject_Folder(m_device, handle, &p_handles, &p_len) != PTP_RC_OK) {
            qWarning("Cannot get folder handles for handle %d", handle);
            return PTP_RC_VITA_Invalid_Data;
        }
    } else {

    }

    int ohfi = m_db->getPathId(name, parent_metadata.ohfi);

    if(ohfi > 0) {
        const QString fullpath = m_db->getAbsolutePath(ohfi);
        qDebug() << "Deleting" << fullpath;
        removeRecursively(fullpath);
        m_db->deleteEntry(ohfi);
    }

    QString fullpath = m_db->getAbsolutePath(parent_metadata.ohfi);
    QDir dir(fullpath);

    if(dataType & Folder) {
        if(!dir.mkpath(name)) {
            qWarning("Cannot create directory: %s", name);
            free(name);
            return PTP_RC_VITA_Failed_Operate_Object;
        }
    } else {
        m_file = new QFile(dir.absoluteFilePath(name));

        if(!m_file->open(QIODevice::WriteOnly)) {
            qWarning("Cannot write to %s", name);
            free(name);
            delete m_file;
            return PTP_RC_VITA_Invalid_Permission;
        } else {
            // the size gets ignored because we can also get it via info.size()
            uint64_t size;

            VitaMTP_GetObject_Callback(m_device, handle, &size, CmaEvent::writeCallback);
            m_file->close();
            delete m_file;
        }
    }

    int new_ohfi = m_db->insertObjectEntry(fullpath, name, parent_metadata.ohfi);
    qDebug("Added object %s with OHFI %i to database", name, new_ohfi);
    free(name);

    if(dataType & Folder) {
        metadata_t folder_metadata;
        m_db->getObjectMetadata(new_ohfi, folder_metadata);
        folder_metadata.handle = handle;

        for(unsigned int i = 0; i < p_len; i++) {
            quint16 ret = processAllObjects(folder_metadata, p_handles[i]);

            if(ret != PTP_RC_OK) {
                qDebug("Deleteting object with OHFI %d", new_ohfi);
                m_db->deleteEntry(new_ohfi);
                return ret;
            }
        }
    }

    return PTP_RC_OK;
}

void CmaEvent::vitaEventGetTreatObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    treat_object_t treatObject;

    if(VitaMTP_GetTreatObject(m_device, eventId, &treatObject) != PTP_RC_OK) {
        qWarning("Cannot get information on object to get");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    metadata_t metadata;

    if(!m_db->getObjectMetadata(treatObject.ohfiParent, metadata)) {
        qWarning("Cannot find parent OHFI %d", treatObject.ohfiParent);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    VitaMTP_ReportResult(m_device, eventId, processAllObjects(metadata, treatObject.handle));
}

void CmaEvent::vitaEventSendCopyConfirmationInfo(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    copy_confirmation_info_t *info;
    if(VitaMTP_SendCopyConfirmationInfoInit(m_device, eventId, &info) != PTP_RC_OK) {
        qWarning("Error receiving initial information.");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    qint64 total_size = 0;

    // check if the item is a single folder
    if(info->count == 1) {
        metadata_t obj_meta;
        if(m_db->getObjectMetadata(info->ohfi[0], obj_meta)) {
            // got a folder
            if(obj_meta.dataType & Folder) {
                metadata_t *meta_list = NULL;
                if(m_db->getObjectList(info->ohfi[0], &meta_list)) {
                    int count = 0;
                    metadata_t *single_meta = meta_list;
                    // count files
                    while(single_meta) {
                        if(single_meta->dataType & File) {
                            count++;
                        }
                        single_meta = single_meta->next_metadata;
                    }
                    // create struct to hold all the file identifiers
                    info = (copy_confirmation_info_t *)malloc(sizeof(uint32_t) * count + sizeof(copy_confirmation_info_t));
                    single_meta = meta_list;
                    info->count = 0;
                    // copy all the file ohfi
                    while(single_meta) {
                        if(single_meta->dataType & File) {
                            info->ohfi[info->count] = single_meta->ohfi;
                            total_size += single_meta->size;
                            info->count++;
                        }
                        single_meta = single_meta->next_metadata;
                    }

                }
                m_db->freeMetadata(meta_list);
            }
        }
    } else {
        qint64 size;

        for(quint32 i = 0; i < info->count; i++) {
            if((size = m_db->getObjectSize(info->ohfi[i])) < 0) {
                qWarning("Cannot find OHFI %d", info->ohfi[i]);
                free(info);
                return;
            }

            total_size += size;
        }
    }

    if(VitaMTP_SendCopyConfirmationInfo(m_device, eventId, info, total_size) != PTP_RC_OK) {
        qWarning("Error sending copy confirmation");
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }

    free(info);
}

void CmaEvent::vitaEventSendObjectMetadataItems(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    quint32 ohfi;
    if(VitaMTP_SendObjectMetadataItems(m_device, eventId, &ohfi) != PTP_RC_OK) {
        qWarning("Cannot get OHFI for retreving metadata");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    metadata_t metadata;

    if(!m_db->getObjectMetadata(ohfi, metadata)) {
        qWarning("Cannot find OHFI %d in database", ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    metadata.next_metadata = NULL;
    qDebug("Sending metadata for OHFI %d (%s)", ohfi, metadata.path);

    quint16 ret = VitaMTP_SendObjectMetadata(m_device, eventId, &metadata);
    if(ret != PTP_RC_OK) {
        qWarning("Error sending metadata. Code: %04X", ret);
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }
}

void CmaEvent::vitaEventSendNPAccountInfo(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);
    // AFAIK, Sony hasn't even implemented this in their CMA
    qWarning("Event 0x%x unimplemented!", cma_event->Code);
}

void CmaEvent::vitaEventUnimplementated(vita_event_t *cma_event, int eventId)
{
    qWarning("Unknown event not handled, code: 0x%x, id: %d", cma_event->Code, eventId);
    qWarning("Param1: 0x%08X, Param2: 0x%08X, Param3: 0x%08X", cma_event->Param1, cma_event->Param2, cma_event->Param3);
}

void CmaEvent::vitaEventCancelTask(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    quint32 eventIdToCancel = cma_event->Param2;
    qDebug("Cancelling event %d", eventIdToCancel);
    quint16 ret = VitaMTP_CancelTask(m_device, eventIdToCancel);

    // wait until the current event finishes so we can report the result to the device

    qDebug("Waiting for send event to finish");
    QMutexLocker locker(&mutex);
    if(ret == PTP_RC_OK) {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }
}

void CmaEvent::vitaEventSendNumOfObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    QMutexLocker locker(&m_db->mutex);

    int ohfi = cma_event->Param2;
    int items = m_db->childObjectCount(ohfi);

    if(VitaMTP_SendNumOfObject(m_device, eventId, items) != PTP_RC_OK) {
        qWarning("Error occurred receiving object count for OHFI parent %d", ohfi);
    } else {
        qDebug("Returned count of %d objects for OHFI parent %d", items, ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }
}

void CmaEvent::vitaEventSendObjectMetadata(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    browse_info_t browse;

    if(VitaMTP_GetBrowseInfo(m_device, eventId, &browse) != PTP_RC_OK) {
        qWarning("GetBrowseInfo failed");
        return;
    }
    QMutexLocker locker(&m_db->mutex);

    metadata_t *meta = NULL;

    int count = m_db->getObjectMetadatas(browse.ohfiParent, &meta, browse.index, browse.numObjects); // if meta is null, will return empty XML
    qDebug("Sending %i metadata filtered objects for OHFI %d", count, browse.ohfiParent);

    if(VitaMTP_SendObjectMetadata(m_device, eventId, meta) != PTP_RC_OK) {  // send all objects with OHFI parent
        qWarning("Sending metadata for OHFI parent %d failed", browse.ohfiParent);
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }
    m_db->freeMetadata(meta);
}

void CmaEvent::vitaEventSendObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    int ohfi = cma_event->Param2;

    QMutexLocker locker(&m_db->mutex);

    qDebug("Searching object with OHFI %d", ohfi);

    metadata_t *metadata = NULL;
    if(!m_db->getObjectList(ohfi, &metadata)) {
        qWarning("Failed to find OHFI %d", ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    metadata_t *start = metadata;
    quint32 parentHandle = cma_event->Param3;
    bool send_folder = metadata->dataType & Folder;
    quint32 handle;

    do {
        quint64 len = metadata->size;
        m_file = new QFile(m_db->getAbsolutePath(metadata->ohfi));

        // read the file to send if it's not a directory
        // if it is a directory, data and len are not used by VitaMTP
        if(metadata->dataType & File) {
            if(!m_file->open(QIODevice::ReadOnly)) {
                qWarning() << "Failed to read" << m_file->fileName();
                delete m_file;
                VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Not_Exist_Object);
                return;
            }
        }

        // get the PTP object ID for the parent to put the object
        // we know the parent has to be before the current node
        // the first time this is called, parentHandle is left untouched

        if(start != metadata) {
            metadata_t parent_metadata;
            m_db->getObjectMetadata(metadata->ohfiParent, parent_metadata);
            parentHandle = parent_metadata.handle;
        }

        // send the data over
        qDebug("Sending %s of %llu bytes to device", metadata->name, len);
        qDebug("OHFI %d with handle 0x%08X", metadata->ohfi, parentHandle);

        VitaMTP_RegisterCancelEventId(eventId);
        quint16 ret = VitaMTP_SendObject_Callback(m_device, &parentHandle, &handle, metadata, &CmaEvent::readCallback);
        if(ret != PTP_RC_OK) {
            qWarning("Sending of %s failed. Code: %04X", metadata->name, ret);
            if(ret == PTP_ERROR_CANCEL) {
                VitaMTP_ReportResult(m_device, eventId, PTP_RC_GeneralError);
            }
            m_file->close();
            delete m_file;
            return;
        }

        metadata->handle = handle;

        if(metadata->dataType & File) {
            m_file->close();
            delete m_file;
        }

        // break early if only a file needs to be sent
        if(!send_folder) {
            break;
        }

        metadata = metadata->next_metadata;

    } while(metadata && metadata->ohfiParent >= OHFI_BASE_VALUE); // get everything under this "folder"

    m_db->freeMetadata(start);

    VitaMTP_ReportResultWithParam(m_device, eventId, PTP_RC_OK, handle);

    VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Data);  // TODO: Send thumbnail
}

void CmaEvent::vitaEventSendHttpObjectFromURL(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    char *url;
    if(VitaMTP_GetUrl(m_device, eventId, &url) != PTP_RC_OK) {
        qWarning("Failed to receive URL");
        return;
    }

    QString urlpath = QSettings().value("urlPath").toString();
    QString basename = QFileInfo(QUrl(url).path()).fileName();
    QFile file(QDir(urlpath).absoluteFilePath(basename));

    QByteArray data;
    QSettings settings;

    bool offlineMode = settings.value("offlineMode", true).toBool();
    bool ignorexml = settings.value("ignorexml", true).toBool();
    bool ignorefile = false;

    // do not try to open the xml file if the flag is set
    if(basename == "psp2-updatelist.xml" && ignorexml) {
        ignorefile = true;
    }

    if(ignorefile || !file.open(QIODevice::ReadOnly)) {
        if(offlineMode && basename == "psp2-updatelist.xml") {
            qDebug("Found request for update list. Sending embedded xml file");
            QFile res(":/main/resources/xml/psp2-updatelist.xml");
            res.open(QIODevice::ReadOnly);
            data = res.readAll();

            // fetch country code from url
            QString countryCode;
            QStringList parts = QUrl(url).path().split('/');
            if(parts.size() >= 2) {
                parts.removeLast();
                countryCode = parts.last();
                qDebug() << "Detected country code from URL: " << countryCode;

                if(countryCode != "us") {
                    QString regionTag = QString("<region id=\"%1\">").arg(countryCode);
                    data.replace("<region id=\"us\">", qPrintable(regionTag));
                }

                QString versiontype = settings.value("versiontype", "zero").toString();

                if(versiontype == "henkaku") {
                    qDebug("Setting XML version to 03.600.000");
                    data.replace("00.000.000", "03.600.000");
                    data.replace("label=\"0.00\"", "label=\"3.60\"");
                } else if(versiontype == "custom") {
                    QString customVersion = settings.value("customversion", "00.000.000").toString();
                    QString customLabel = QString("label=\"%1\"").arg(customVersion.mid(1, 4));
                    qDebug("Setting XML version to %s", qPrintable(customVersion));
                    data.replace("00.000.000", qPrintable(customVersion));
                    data.replace("label=\"0.00\"", qPrintable(customLabel));
                } else {
                    qDebug("Using default XML version: 00.000.000");
                }

            } else {
                qWarning() << "No country code found in URL, defaulting to \"us\"";
            }

        } else if(!offlineMode) {
            qDebug("URL: %s", url);
            messageSent(tr("The PSVita has requested an update check, downloading from %1").arg(url));
            HTTPDownloader downloader(url);
            QThread *http_thread = new QThread();
            http_thread->setObjectName("http_thread");
            connect(http_thread, SIGNAL(started()), &downloader, SLOT(downloadFile()));
            connect(&downloader, SIGNAL(messageSent(QString)), SIGNAL(messageSent(QString)), Qt::DirectConnection);
            downloader.moveToThread(http_thread);
            http_thread->start();

            int remote_size = (int)downloader.getFileSize();

            if(remote_size != -1) {
                // add the size of the file length to the total filesize
                remote_size += 8;
                qDebug("Sending %i bytes of data for HTTP request %s", remote_size, url);

                if(VitaMTP_SendData_Callback(m_device, eventId, PTP_OC_VITA_SendHttpObjectFromURL, remote_size, HTTPDownloader::readCallback) != PTP_RC_OK) {
                    qWarning("Failed to send HTTP object");
                } else {
                    VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
                }

            } else {
                qWarning("No valid content-length in header, aborting");
                VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Download);
            }

            free(url);
            http_thread->quit();
            http_thread->deleteLater();
            return;
        }
    } else {
        qDebug("Reading from local file");
        data = file.readAll();

        if(basename == "psp2-updatelist.xml" && !ignorefile) {
            messageSent(tr("The PSVita has requested an update check, sending local xml file and ignoring version settings"));
        } else {
            QString versiontype = settings.value("versiontype", "zero").toString();
            QString customVersion = settings.value("customversion", "00.000.000").toString();

            // verify that the update file is really the 3.60 pup
            // to prevent people updating to the wrong version and lose henkaku.
            if(ignorexml && basename == "PSP2UPDAT.PUP" &&
                    (versiontype == "henkaku" ||
                    (versiontype == "custom" &&
                    customVersion == "03.600.000"))) {
                QCryptographicHash crypto(QCryptographicHash::Sha256);
                crypto.addData(data);
                QString result = crypto.result().toHex();

                if(result != hash360) {
                    qWarning("3.60 PUP SHA256 mismatch");
                    qWarning("> Actual:   %s", qPrintable(result));
                    qWarning("> Expected: %s", qPrintable(hash360));
                    // notify the user
                    messageSent(tr("The XML version is set to 3.60 but the PUP file hash doesn't match, cancel the update if you don't want this"));
                }
            }
        }
    }

    qDebug("Sending %i bytes of data for HTTP request %s", data.size(), url);

    if(VitaMTP_SendHttpObjectFromURL(m_device, eventId, data.data(), data.size()) != PTP_RC_OK) {
        qWarning("Failed to send HTTP object");
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }

    free(url);
}

void CmaEvent::vitaEventSendObjectStatus(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    object_status_t objectstatus;

    if(VitaMTP_SendObjectStatus(m_device, eventId, &objectstatus) != PTP_RC_OK) {
        qWarning("Failed to get information for object status.");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    qDebug("Checking for path %s under ohfi %i", objectstatus.title, objectstatus.ohfiRoot);
    int ohfi = m_db->getPathId(objectstatus.title, objectstatus.ohfiRoot);

    if(ohfi == 0) { // not in database, don't return metadata
        qDebug("Object %s not in database (OHFI: %i). Sending OK response for non-existence", objectstatus.title, objectstatus.ohfiRoot);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    } else {
        metadata_t metadata = metadata_t();
        m_db->getObjectMetadata(ohfi, metadata);
        metadata.next_metadata = NULL;
        qDebug("Sending metadata for OHFI %d", ohfi);

        if(VitaMTP_SendObjectMetadata(m_device, eventId, &metadata) != PTP_RC_OK) {
            qWarning("Error sending metadata for %d", ohfi);
        } else {
            VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
        }
    }

    free(objectstatus.title);
}

void CmaEvent::vitaEventSendObjectThumb(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    QMutexLocker locker(&m_db->mutex);

    int ohfi = cma_event->Param2;
    metadata_t metadata;

    if(!m_db->getObjectMetadata(ohfi, metadata)) {
        qWarning("Cannot find OHFI %d in database.", ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    QString fullpath = m_db->getAbsolutePath(ohfi);
    QByteArray data = getThumbnail(fullpath, metadata.dataType, &g_thumbmeta);

    if(data.size() == 0) {
        qWarning() << "Cannot find/read thumbnail for" << fullpath;
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Data);
        return;
    }

    //FIXME: remove this after fixing vitamtp

    // workaround for the vitamtp locale bug
    char *locale = strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "C");

    if(VitaMTP_SendObjectThumb(m_device, eventId, (metadata_t *)&g_thumbmeta, (uchar *)data.data(), data.size()) != PTP_RC_OK) {
        qWarning("Error sending thumbnail");
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }

    // restore locale
    setlocale(LC_ALL, locale);
    free(locale);
}

void CmaEvent::vitaEventDeleteObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    QMutexLocker locker(&m_db->mutex);

    int ohfi = cma_event->Param2;
    metadata_t metadata;

    if(!m_db->getObjectMetadata(ohfi, metadata)) {
        qWarning("OHFI %d not found", ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    }

    QString fullpath = m_db->getAbsolutePath(ohfi);
    qDebug() << QString("Deleting %1, OHFI: %2").arg(fullpath, QString::number(ohfi));
    removeRecursively(fullpath);
    m_db->deleteEntry(ohfi);

    VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
}

void CmaEvent::vitaEventGetSettingInfo(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    settings_info_t *settingsinfo;
    if(VitaMTP_GetSettingInfo(m_device, eventId, &settingsinfo) != PTP_RC_OK) {
        qWarning("Failed to get setting info from Vita.");
        return;
    }

    qDebug("Current account id: %s", settingsinfo->current_account.accountId);

    QSettings settings;

    if(settings.value("lastAccountId").toString() != settingsinfo->current_account.accountId) {
        m_db->setUUID(settingsinfo->current_account.accountId);
        // set the database to be updated ASAP
        emit refreshDatabase();
    }

    // free all the information
    VitaMTP_Data_Free_Settings(settingsinfo);
    VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
}

void CmaEvent::vitaEventSendHttpObjectPropFromURL(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    char *url;
    if(VitaMTP_GetUrl(m_device, eventId, &url) != PTP_RC_OK) {
        qWarning("Failed to get URL");
        return;
    }

    QString urlpath = QSettings().value("urlPath").toString();
    QString basename = QFileInfo(url).fileName();
    QFileInfo file(QDir(urlpath).absoluteFilePath(basename));

    if(!file.exists()) {
        qWarning("The file %s is not accesible", url);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Download);
        free(url);
        return;
    }

    QString timestamp = file.lastModified().toString();

    http_object_prop_t httpobjectprop;
    httpobjectprop.timestamp = timestamp.toUtf8().data();
    httpobjectprop.timestamp_len = timestamp.toUtf8().size();

    if(VitaMTP_SendHttpObjectPropFromURL(m_device, eventId, &httpobjectprop) != PTP_RC_OK) {
        qWarning("Failed to send object properties");
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }

    free(url);
}

void CmaEvent::vitaEventSendPartOfObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    send_part_init_t part_init;

    if(VitaMTP_SendPartOfObjectInit(m_device, eventId, &part_init) != PTP_RC_OK) {
        qWarning("Cannot get information on object to send");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    QString fullpath = m_db->getAbsolutePath(part_init.ohfi);

    if(fullpath.isNull()) {
        qWarning("Cannot find object for OHFI %d", part_init.ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Context);
        return;
    }

    QFile file(fullpath);

    if(!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot read" << fullpath;
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    } else {
        file.seek(part_init.offset);
        QByteArray data = file.read(part_init.size);
        qDebug() << QString("Sending %1 at file offset %2 for %3 bytes").arg(
                     fullpath, QString::number(part_init.offset), QString::number(part_init.size)
                 );

        if(VitaMTP_SendPartOfObject(m_device, eventId, (unsigned char *)data.data(), data.size()) != PTP_RC_OK) {
            qWarning("Failed to send part of object OHFI %d", part_init.ohfi);
        } else {
            VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
        }
    }
}

void CmaEvent::vitaEventOperateObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    operate_object_t operateobject;

    if(VitaMTP_OperateObject(m_device, eventId, &operateobject) != PTP_RC_OK) {
        qWarning("Cannot get information on object to operate");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    QString fullpath = m_db->getAbsolutePath(operateobject.ohfi);

    // end for renaming only
    if(fullpath.isNull()) {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Not_Exist_Object);
        return;
    }

    switch(operateobject.cmd) {
    case VITA_OPERATE_CREATE_FOLDER: {
        qDebug("Operate command %d: Create folder %s", operateobject.cmd, operateobject.title);

        QDir dir(fullpath);
        if(!dir.mkdir(operateobject.title)) {
            qWarning("Unable to create temporary folder: %s", operateobject.title);
            VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        } else {
            int new_ohfi = m_db->insertObjectEntry(fullpath, operateobject.title, operateobject.ohfi);
            qDebug("Created folder %s with OHFI %d", operateobject.title, new_ohfi);
            VitaMTP_ReportResultWithParam(m_device, eventId, PTP_RC_OK, new_ohfi);
        }
        break;
    }
    case VITA_OPERATE_CREATE_FILE: {
        qDebug("Operate command %d: Create file %s", operateobject.cmd, operateobject.title);

        QFile file(fullpath + QDir::separator() + operateobject.title);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("Unable to create temporary file: %s", operateobject.title);
            VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        } else {
            int new_ohfi = m_db->insertObjectEntry(fullpath, operateobject.title, operateobject.ohfi);
            //qDebug("Created file %s with OHFI %d under parent %s", newobj->metadata.path, new_ohfi, root->metadata.path);
            VitaMTP_ReportResultWithParam(m_device, eventId, PTP_RC_OK, new_ohfi);
        }
        break;
    }
    case VITA_OPERATE_RENAME: {
        //qDebug("Operate command %d: Rename %s to %s", operateobject.cmd, root->metadata.name, operateobject.title);

        m_db->renameObject(operateobject.ohfi, operateobject.title);
        QString newpath = m_db->getAbsolutePath(operateobject.ohfi);

        // rename in filesystem
        if(!QFile(fullpath).rename(newpath)) {
            qWarning("Unable to rename %s to %s", fullpath.toLocal8Bit().constData(), operateobject.title);
            VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Operate_Object);
            break;
        }

        qDebug("Renamed OHFI %d from %s to %s", operateobject.ohfi, fullpath.toLocal8Bit().constData(), newpath.toLocal8Bit().constData());
        VitaMTP_ReportResultWithParam(m_device, eventId, PTP_RC_OK, operateobject.ohfi);
        break;
    }

    default:
        qWarning("Operate command %d: Not implemented", operateobject.cmd);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Failed_Operate_Object);
        break;
    }

    free(operateobject.title);
}

void CmaEvent::vitaEventGetPartOfObject(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    unsigned char *data;
    send_part_init_t part_init;

    if(VitaMTP_GetPartOfObject(m_device, eventId, &part_init, &data) != PTP_RC_OK) {
        qWarning("Cannot get object from device");
        return;
    }

    QMutexLocker locker(&m_db->mutex);
    QString fullpath = m_db->getAbsolutePath(part_init.ohfi);

    if(fullpath.isNull()) {
        qWarning("Cannot find OHFI %d", part_init.ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        free(data);
        return;
    }

    qDebug() << QString("Receiving %1 at offset %2 for %3 bytes").arg(
                 fullpath, QString::number(part_init.offset), QString::number(part_init.size)
             );

    QFile file(fullpath);
    if(!file.open(QIODevice::ReadWrite)) {
        qWarning() << "Cannot write to file" << fullpath;
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Permission);
    } else {
        file.seek(part_init.offset);
        file.write((const char *)data, part_init.size);
        m_db->setObjectSize(part_init.ohfi, part_init.size);

        qDebug() << QString("Written %1 bytes to %2 at offset %3").arg(
                     QString::number(part_init.size), fullpath, QString::number(part_init.offset)
                 );

        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }

    free(data);
}

void CmaEvent::vitaEventSendStorageSize(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    QMutexLocker locker(&m_db->mutex);

    int ohfi = cma_event->Param2;
    QString fullpath = m_db->getAbsolutePath(ohfi);

    if(fullpath.isNull()) {
        qWarning("Error: Cannot find OHFI %d", ohfi);
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_OHFI);
        return;
    } else {
        QFile file(fullpath);

        if(!file.exists()) {
            // create the directory if doesn't exist so the query don't fail
            qDebug() << "Creating" << fullpath;

            if(!QDir(QDir::root()).mkpath(QDir(fullpath).absolutePath())) {
                qWarning("Create directory failed");
                VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Permission);
                return;
            }
        }
    }

    quint64 total;
    quint64 free;

    if(!getDiskSpace(fullpath, &free, &total)) {
        qWarning("Cannot get disk space");
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Invalid_Permission);
        return;
    }

    qDebug("Storage stats for drive containing OHFI %d, free: %llu, total: %llu", ohfi, free, total);

    if(VitaMTP_SendStorageSize(m_device, eventId, total, free) != PTP_RC_OK) {
        qWarning("Send storage size failed");
    } else {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
    }
}

void CmaEvent::vitaEventCheckExistance(vita_event_t *cma_event, int eventId)
{
    qDebug("Event received in %s, code: 0x%x, id: %d", Q_FUNC_INFO, cma_event->Code, eventId);

    int handle = cma_event->Param2;
    existance_object_t existance;

    if(VitaMTP_CheckExistance(m_device, handle, &existance) != PTP_RC_OK) {
        qWarning("Cannot read information on object to be sent");
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    int ohfi = m_db->getPathId(existance.name, 0);

    if(ohfi == 0) {
        VitaMTP_ReportResult(m_device, eventId, PTP_RC_VITA_Different_Object);
    } else {
        VitaMTP_ReportResultWithParam(m_device, eventId, PTP_RC_VITA_Same_Object, ohfi);
    }

    VitaMTP_ReportResult(m_device, eventId, PTP_RC_OK);
}

int CmaEvent::readCallback(unsigned char *data, unsigned long wantlen, unsigned long *gotlen)
{
    int ret = m_file->read((char *)data, wantlen);

    if(ret != -1) {
        *gotlen = ret;
        ret = PTP_RC_OK;
    }

    return ret;
}

int CmaEvent::writeCallback(const unsigned char *data, unsigned long size, unsigned long *written)
{
    int ret = m_file->write((const char *)data, size);

    if(ret != -1) {
        *written = ret;
        ret = PTP_RC_OK;
    }

    return ret;
}
