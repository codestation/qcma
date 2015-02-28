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

#ifndef CMAEVENT_H
#define CMAEVENT_H

#include "database.h"
#include "httpdownloader.h"

#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QSemaphore>

#include <vitamtp.h>

class CmaEvent : public QObject
{
    Q_OBJECT
public:
    explicit CmaEvent(Database *db, vita_device_t *s_device);
    void vitaEventCancelTask(vita_event_t *event, int eventId);

private:
    uint16_t processAllObjects(metadata_t &metadata, uint32_t handle);
    void vitaEventSendObject(vita_event_t *event, int eventId);
    void vitaEventSendObjectMetadata(vita_event_t *event, int eventId);
    void vitaEventSendNumOfObject(vita_event_t *event, int eventId);
    void vitaEventSendHttpObjectFromURL(vita_event_t *event, int eventId);
    void vitaEventUnimplementated(vita_event_t *event, int eventId);
    void vitaEventSendObjectStatus(vita_event_t *event, int eventId);
    void vitaEventSendObjectThumb(vita_event_t *event, int eventId);
    void vitaEventDeleteObject(vita_event_t *event, int eventId);
    void vitaEventGetSettingInfo(vita_event_t *event, int eventId);
    void vitaEventSendHttpObjectPropFromURL(vita_event_t *event, int eventId);
    void vitaEventSendPartOfObject(vita_event_t *event, int eventId);
    void vitaEventOperateObject(vita_event_t *event, int eventId);
    void vitaEventGetPartOfObject(vita_event_t *event, int eventId);
    void vitaEventSendStorageSize(vita_event_t *event, int eventId);
    void vitaEventCheckExistance(vita_event_t *event, int eventId);
    void vitaEventGetTreatObject(vita_event_t *event, int eventId);
    void vitaEventSendCopyConfirmationInfo(vita_event_t *event, int eventId);
    void vitaEventSendObjectMetadataItems(vita_event_t *event, int eventId);
    void vitaEventSendNPAccountInfo(vita_event_t *event, int eventId);

    static int readCallback(unsigned char *data, unsigned long wantlen, unsigned long *gotlen);
    static int writeCallback(const unsigned char *data, unsigned long size, unsigned long *written);

    void processEvent();
    bool isActive();
    void setDevice(vita_device_t *m_device);

    vita_device_t *m_device;
    vita_event_t m_event;

    Database *m_db;

    // control variables
    bool is_active;
    QMutex mutex;
    QMutex active;
    QSemaphore sema;

    static QFile *m_file;

signals:
    void finishedEventLoop();
    void refreshDatabase();
    void messageSent(QString);

public slots:
    void process();
    void setEvent(vita_event_t event);
    void stop();
};

#endif // CMAEVENT_H
