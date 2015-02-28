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

#include "qlistdb.h"
#include "cmaevent.h"
#include "cmabroadcast.h"

#include <QObject>
#include <QSemaphore>
#include <QString>
#include <QWaitCondition>

#include <vitamtp.h>

class CmaClient : public QObject
{
    Q_OBJECT
public:
    explicit CmaClient(Database *db, QObject *obj_parent = 0);
    explicit CmaClient(Database *db, CmaBroadcast *broadcast, QObject *obj_parent = 0);

    static bool isRunning();
    void launch();

private:
    static bool isActive();
    static void setActive(bool state);
    static bool isEventLoopEnabled();
    static void setEventLoop(bool state);
    void enterEventLoop(vita_device_t *device);

    void processNewConnection(vita_device_t *device);

    static int deviceRegistered(const char *deviceid);
    static int generatePin(wireless_vita_info_t *info, int *p_err);
    static int cancelCallback();
    static void registrationComplete();

    Database *m_db;
    CmaBroadcast *m_broadcast;
    static QString tempOnlineId;

    //TODO: move all the control variables to the client manager class
    static bool is_active;
    static bool in_progress;
    static CmaClient *this_object;
    static QMutex mutex;
    static QMutex runner;
    static QMutex usbwait;
    static QWaitCondition usbcondition;
    static QSemaphore sema;

signals:
    void newEvent(vita_event_t event);
    void receivedPin(QString, int);
    void pinComplete();
    void deviceDetected();
    void deviceConnected(QString);
    void messageSent(QString);
    void deviceDisconnected();
    void refreshDatabase();
    void finished();

public slots:
    static int stop();

private slots:
    void connectUsb();
    void connectWireless();
};

#endif // CMACLIENT_H
