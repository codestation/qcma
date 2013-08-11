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

#include "qcma.h"

#include "capability.h"
#include "listenerworker.h"
#include "wirelessworker.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QHostInfo>

#include <unistd.h>

#define RETRY_TIMEOUT 2000

QCMA::QCMA(QObject *parent) :
    BaseWorker(parent)
{
    exit_thread = false;
}

vita_device_t *QCMA::connectWireless()
{
    int num_tries = 0;
    vita_device_t *vita = NULL;
    WirelessWorker *broadcast = new WirelessWorker();

    // bubble up the pin received signal
    connect(broadcast, SIGNAL(createdPin(int)), this, SIGNAL(createdPin(int)));

    qDebug("Starting broadcast");
    broadcast->start();

    QMutex mutex;
    QString hostname = QHostInfo::localHostName();
    WirelessWorker::info.name = hostname.toUtf8().data();

    while(!break_loop && (vita = VitaMTP_Get_First_Wireless_Vita(
                                      &WirelessWorker::info, 0, 10,
                                      WirelessWorker::deviceRegistered,
                                      WirelessWorker::generatePin)) == NULL) {
        qDebug("Error connecting to device. Attempt %d", ++num_tries);
        //FIXME: use a proper sleep function
        mutex.lock();
        waitCondition.wait(&mutex, RETRY_TIMEOUT);
        mutex.unlock();
    }
    qDebug("Stopping broadcast");
    broadcast->stopBroadcast();
    return vita;
}

vita_device_t *QCMA::connectUsb()
{
    int num_tries = 0;
    vita_device_t *vita = NULL;
    QMutex mutex;
    while(!break_loop && (vita = VitaMTP_Get_First_USB_Vita()) == NULL) {
        qDebug("No Vita found. Attempt %d", ++num_tries);
        //FIXME: use a proper sleep function
        mutex.lock();
        waitCondition.wait(&mutex, RETRY_TIMEOUT);
        mutex.unlock();
    }

    return vita;
}

void QCMA::process()
{    
    QSettings settings;

    qDebug() << "From QCMA: "<< QThread::currentThreadId();

    while(!exit_thread) {
        break_loop = false;
        VitaMTP_Set_Logging(VitaMTP_NONE);
        bool wireless = settings.value("wireless", false).toBool();
        emit statusUpdated("Searching for device...");
        vita_device_t *device = wireless ? connectWireless() : connectUsb();
        if(!device) {
            continue;
        }

        qDebug("Vita connected: id %s", VitaMTP_Get_Identification(device));

        // create a listener thread to receive the vita events
        ListenerWorker *listener = new ListenerWorker();
        listener->setDevice(device);
        connect(listener, SIGNAL(refreshDatabase()), this, SLOT(allowRefresh()), Qt::DirectConnection);
        connect(listener, SIGNAL(finished()), this, SIGNAL(deviceDisconnected()), Qt::DirectConnection);
        connect(listener, SIGNAL(finished()), this, SLOT(reload()), Qt::DirectConnection);
        listener->start();
        qDebug("Exchanging info with vita");
        DeviceCapability *vita_info = new DeviceCapability();

        if(!vita_info->exchangeInfo(device)) {
            qCritical("Error while exchanging info with the vita");
            VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
            VitaMTP_Release_Device(device);
            sleep(RETRY_TIMEOUT);
            continue;
        }

        // Conection successful, inform the user
        emit deviceConnected(QString(tr("Connected to ")) + vita_info->getOnlineId());

        delete vita_info;
        QElapsedTimer timer;
        qint64 sec;

        while(listener->isConnected()) {
            sema.acquire();
            if(break_loop) {
                break;
            }
            qDebug("Updating database");
            timer.start();
            int scanned_objects = listener->rebuildDatabase();
            sec = timer.elapsed();
            qDebug("Scanned %i objects in %lli milliseconds", scanned_objects, sec);
            emit statusUpdated(QString("Scanned %1 objects in %2 milliseconds").arg(scanned_objects, sec));
        }

        qDebug("Ending device connection...");
        VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
        VitaMTP_Release_Device(device);
    }
    qDebug("QCMA thread end");
    emit finished();
}

void QCMA::allowRefresh()
{
    sema.release();
}

void QCMA::reload()
{
    qDebug("Stopping QCMA workers...");
    break_loop = true;
    waitCondition.wakeAll();
    sema.release();
}

void QCMA::stop()
{    
    exit_thread = true;
    reload();
    qDebug("Stopping QCMA thread...");
    emit stopCMA();
}
