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

#include "wirelessworker.h"
#include "utils.h"

#include <QTime>
#include <QHostInfo>

#define QCMA_REQUEST_PORT 9309

wireless_host_info_t BroadcastSignal::info = {"00000000-0000-0000-0000-000000000000", "win", NULL, QCMA_REQUEST_PORT};

BroadcastSignal *BroadcastSignal::this_object = NULL;

BroadcastSignal::BroadcastSignal(QObject *parent) :
    BaseWorker(parent)
{
    qsrand(QTime::currentTime().msec());
    this_object = this;
    started = false;
}

BroadcastSignal::~BroadcastSignal()
{
    VitaMTP_Stop_Broadcast();
}

int BroadcastSignal::deviceRegistered(const char *deviceid)
{
    qDebug("Got connection request from %s", deviceid);
    return 1;
}

int BroadcastSignal::generatePin(wireless_vita_info_t *info, int *p_err)
{
    qDebug("Registration request from %s (MAC: %s)", info->name, info->mac_addr);
    int pin = qrand() % 100000000;
    qDebug("Your registration PIN for %s is: %08d", info->name, pin);
    *p_err = 0;
    emit this_object->createdPin(pin);
    return pin;
}

void BroadcastSignal::process()
{
    qDebug() <<"Broadcast thread ID: "<< QThread::currentThreadId();
    qDebug("Starting CMA wireless broadcast...");

    started = true;
    if(VitaMTP_Broadcast_Host(&info, 0) < 0) {
        qCritical( "An error occured during broadcast. Exiting.");
    } else {
        qDebug("Broadcast ended.");
    }
    qDebug("Broadcast thread end.");
    emit finished();
}

void BroadcastSignal::stopBroadcast()
{
    qDebug("Stopping broadcast");
    VitaMTP_Stop_Broadcast();
    started = false;
}
