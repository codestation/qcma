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

#include "cmaclient.h"
#include "capability.h"
#include "avdecoder.h"
#include "cmaevent.h"
#include "utils.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QUrl>

QMutex CmaClient::mutex;
QMutex CmaClient::runner;
QMutex CmaClient::cancel;
QSemaphore CmaClient::sema;

bool CmaClient::is_active = true;
bool CmaClient::in_progress = false;
int CmaClient::is_cancelled = false;

CmaClient *CmaClient::this_object = NULL;

CmaClient::CmaClient(QObject *parent) :
    QObject(parent)
{
    this_object = this;
}

void CmaClient::connectUsb()
{
    vita_device_t *vita;

    qDebug() << "Starting usb_thread:" << QThread::currentThreadId();

    do {
        if((vita = VitaMTP_Get_First_USB_Vita()) !=NULL) {
            processNewConnection(vita);
            VitaMTP_Close_USB_Vita();
        } else {
            //TODO: replace this with an event-driven setup
            Sleeper::msleep(2000);
            mutex.lock();
            if(in_progress) {
                sema.acquire();
            }
            mutex.unlock();
        }
    } while(isActive());

    qDebug("Finishing usb_thread");
    emit finished();
}

void CmaClient::connectWireless()
{
    vita_device_t *vita;
    wireless_host_info_t host = {NULL, NULL, NULL, QCMA_REQUEST_PORT};
    typedef CmaClient CC;

    qDebug() << "Starting wireless_thread:" << QThread::currentThreadId();

    do {
        if((vita = VitaMTP_Get_First_Wireless_Vita(&host, 0, CC::cancelCallback, CC::deviceRegistered, CC::generatePin)) != NULL) {
            processNewConnection(vita);
        } else {
            Sleeper::msleep(2000);
            mutex.lock();
            if(in_progress) {
                sema.acquire();
            }
            mutex.unlock();;
        }
    } while(isActive());

    qDebug("Finishing wireless_thread");
    emit finished();
}

void CmaClient::processNewConnection(vita_device_t *device)
{
    QMutexLocker locker(&mutex);
    in_progress = true;
    broadcast.setUnavailable();

    qDebug("Vita connected: id %s", VitaMTP_Get_Identification(device));
    DeviceCapability *vita_info = new DeviceCapability();

    if(!vita_info->exchangeInfo(device)) {
        qCritical("Error while exchanging info with the vita");
    } else {
        // Conection successful, inform the user
        emit deviceConnected(QString(tr("Connected to ")) + vita_info->getOnlineId());
        enterEventLoop(device);
    }

    VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
    VitaMTP_Release_Device(device);

    emit deviceDisconnected();

    broadcast.setAvailable();
    in_progress = false;
    sema.release();
}

int CmaClient::deviceRegistered(const char *deviceid)
{
    qDebug("Got connection request from %s", deviceid);
    // TODO: check the device to see if is already registered
    return 1;
}

int CmaClient::generatePin(wireless_vita_info_t *info, int *p_err)
{
    qDebug("Registration request from %s (MAC: %s)", info->name, info->mac_addr);
    int pin = qrand() % 100000000;
    qDebug("Your registration PIN for %s is: %08d", info->name, pin);
    *p_err = 0;
    emit this_object->receivedPin(info->name, pin);
    return pin;
}

int CmaClient::cancelCallback()
{
    QMutexLocker locker(&cancel);
    return is_cancelled;
}

void CmaClient::enterEventLoop(vita_device_t *device)
{
    vita_event_t event;

    qDebug("Starting event loop");

    CmaEvent eventLoop (device);
    QThread thread;
    thread.setObjectName("event_thread");

    eventLoop.moveToThread(&thread);
    connect(&thread, SIGNAL(started()), &eventLoop, SLOT(process()));
    connect(&eventLoop, SIGNAL(finishedEventLoop()), &thread, SLOT(quit()), Qt::DirectConnection);
    thread.start();

    while(isActive()) {
        if(VitaMTP_Read_Event(device, &event) < 0) {
            qWarning("Error reading event from Vita.");
            break;
        }

        // do not create a event for this since there aren't more events to read
        if(event.Code == PTP_EC_VITA_RequestTerminate) {
            qDebug("Terminating event thread");
            break;

            // this one shuold be processed inmediately
        } else if(event.Code == PTP_EC_VITA_RequestCancelTask) {
            quint32 eventIdToCancel = event.Param2;
            qDebug("Cancelling event %d", eventIdToCancel);
            VitaMTP_CancelTask(device, eventIdToCancel);
            continue;
        }

        // the events are processed synchronously except for cancel/terminate
        qDebug("Sending new event");
        eventLoop.setEvent(event);
    }

    eventLoop.stop();
    thread.wait();
    qDebug("Finishing event loop");
}

void CmaClient::stop()
{
    CmaClient::setActive(false);
    cancel.lock();
    is_cancelled = true;
    cancel.unlock();
}

bool CmaClient::isActive()
{
    QMutexLocker locker(&runner);
    return is_active;
}

void CmaClient::setActive(bool state)
{
    QMutexLocker locker(&runner);
    is_active = state;
}

