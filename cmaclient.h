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

#ifndef CMACLIENT_H
#define CMACLIENT_H

#include "baseworker.h"
#include "cmabroadcast.h"
#include "cmaobject.h"
#include "database.h"

#include <QObject>
#include <QString>
#include <QWaitCondition>

extern "C" {
#include <vitamtp.h>
}

class CmaClient : public QObject
{
    Q_OBJECT
public:
    explicit CmaClient(QObject *parent = 0);
    ~CmaClient();

    void launch();

    static Database db;

private:
    static bool isRunning();
    static void setRunning(bool state);
    static bool isEventLoopEnabled();
    static void setEventLoop(bool state);
    void enterEventLoop();

    void processNewConnection(vita_device_t *device);

    uint16_t processAllObjects(CMAObject *parent, uint32_t handle);
    void vitaEventSendObject(vita_event_t *event, int eventId);
    void vitaEventSendObjectMetadata(vita_event_t *event, int eventId);
    void vitaEventSendNumOfObject(vita_event_t *event, int eventId);
    void vitaEventCancelTask(vita_event_t *event, int eventId);
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
    void vitaEventRequestTerminate(vita_event_t *event, int eventId);

    static int deviceRegistered(const char *deviceid);
    static int generatePin(wireless_vita_info_t *info, int *p_err);

    int cancel_wireless;
    CmaBroadcast broadcast;
    vita_device_t *device;
    static bool event_loop_enabled;
    static bool is_running;
    static metadata_t g_thumbmeta;
    static CmaClient *this_object;
    static QMutex mutex;
    static QMutex runner;
    static QMutex eloop;

signals:
    void receivedPin(int);
    void deviceDetected();
    void deviceConnected(QString);
    void deviceDisconnected();
    void refreshDatabase();
    void finished();

public slots:
    void close();
    static void stop();

private slots:
    void connectUsb();
    void connectWireless();
};

#endif // CMACLIENT_H
