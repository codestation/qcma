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

#include "QApplication"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QUrl>

QMutex CmaClient::mutex;
QMutex CmaClient::runner;
QMutex CmaClient::eloop;
bool CmaClient::is_running = true;
bool CmaClient::event_loop_enabled = true;

CmaClient *CmaClient::this_object = NULL;

CmaClient::CmaClient(QObject *parent) :
    QObject(parent)
{
    this_object = this;
    device = NULL;
}

bool CmaClient::isRunning()
{
    QMutexLocker locker(&runner);
    return is_running;
}

void CmaClient::setRunning(bool state)
{
    QMutexLocker locker(&runner);
    is_running = state;
}

void CmaClient::connectUsb()
{
    vita_device_t *vita;
    //int num_tries = 0;

    qDebug() << "Starting usb_thread:" << QThread::currentThreadId();

    while(isRunning()) {
        if((vita = VitaMTP_Get_First_USB_Vita()) !=NULL) {
            cancel_wireless = 1;
            processNewConnection(vita);
        } else {
            //qDebug("No Vita detected via USB, attempt %i", +num_tries++);
            if(mutex.tryLock()) {
                mutex.unlock();
                Sleeper::msleep(2000);
            } else {
                mutex.lock();
                mutex.unlock();
            }
        }
    }

    qDebug("Finishing usb_thread");
    emit finished();
}

void CmaClient::connectWireless()
{
    vita_device_t *vita;
    wireless_host_info_t host;
    host.port = QCMA_REQUEST_PORT;
    typedef CmaClient CC;
    cancel_wireless = 0;

    qDebug() << "Starting wireless_thread:" << QThread::currentThreadId();

    while(isRunning()) {
        if((vita = VitaMTP_Get_First_Wireless_Vita(&host, 0, &cancel_wireless, CC::deviceRegistered, CC::generatePin)) != NULL) {
            processNewConnection(vita);
        } else {
            qDebug("Wireless listener was cancelled");            
            // wait until the event loop of the usb thread is finished
            mutex.lock();
            cancel_wireless = 0;
            mutex.unlock();
        }
    }
    qDebug("Finishing wireless_thread");
    emit finished();
}

void CmaClient::processNewConnection(vita_device_t *device)
{
    QMutexLocker locker(&mutex);

    broadcast.setUnavailable();
    this->device = device;
    qDebug("Vita connected: id %s", VitaMTP_Get_Identification(device));
    DeviceCapability *vita_info = new DeviceCapability();

    if(!vita_info->exchangeInfo(device)) {
        qCritical("Error while exchanging info with the vita");
    } else {
        // Conection successful, inform the user
        emit deviceConnected(QString(tr("Connected to ")) + vita_info->getOnlineId());
        enterEventLoop();
    }

    broadcast.setAvailable();
}

int CmaClient::deviceRegistered(const char *deviceid)
{
    qDebug("Got connection request from %s", deviceid);
    return 1;
}

int CmaClient::generatePin(wireless_vita_info_t *info, int *p_err)
{
    qDebug("Registration request from %s (MAC: %s)", info->name, info->mac_addr);
    int pin = qrand() % 100000000;
    qDebug("Your registration PIN for %s is: %08d", info->name, pin);
    *p_err = 0;
    emit this_object->receivedPin(pin);
    return pin;
}

void CmaClient::enterEventLoop()
{
    vita_event_t event;

    qDebug("Starting event loop");

    event_loop_enabled = true;

    while(isEventLoopEnabled()) {
        if(VitaMTP_Read_Event(device, &event) < 0) {
            qWarning("Error reading event from Vita.");
            break;
        }
        CmaEvent *vita_event = new CmaEvent(device, event);
        connect(vita_event, SIGNAL(finishedEventLoop()), this, SLOT(finishEventLoop()));
        connect(vita_event, SIGNAL(refreshDatabase()), this, SIGNAL(refreshDatabase()));
        vita_event->start("cma_event");
    }
}

bool CmaClient::isEventLoopEnabled()
{
    QMutexLocker locker(&eloop);
    return event_loop_enabled;
}

void CmaClient::finishEventLoop()
{
    QMutexLocker locker(&eloop);
    event_loop_enabled = false;
}

void CmaClient::close()
{
    if(device) {
        VitaMTP_SendHostStatus(device, VITA_HOST_STATUS_EndConnection);
        VitaMTP_Release_Device(device);
        device = NULL;
    }
}

void CmaClient::stop()
{
    CmaClient::setRunning(false);
    CmaClient::finishEventLoop();
}

CmaClient::~CmaClient()
{
    close();
}
