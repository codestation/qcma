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

private:
    static bool isRunning();
    static void setRunning(bool state);
    static bool isEventLoopEnabled();
    static void setEventLoop(bool state);
    void enterEventLoop();

    void processNewConnection(vita_device_t *device);

    static int deviceRegistered(const char *deviceid);
    static int generatePin(wireless_vita_info_t *info, int *p_err);

    int cancel_wireless;
    CmaBroadcast broadcast;
    vita_device_t *device;
    static bool event_loop_enabled;
    static bool is_running;
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
    static void finishEventLoop();
};

#endif // CMACLIENT_H
