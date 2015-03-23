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

#include "cmabroadcast.h"
#include "cmautils.h"

#include <QDebug>
#include <QHostAddress>
#include <QHostInfo>
#include <QMutexLocker>
#include <QSettings>
#include <QUuid>

#include <vitamtp.h>

const QString CmaBroadcast::broadcast_reply =
    "%1\r\n"
    "host-id:%2\r\n"
    "host-type:%3\r\n"
    "host-name:%4\r\n"
    "host-mtp-protocol-version:%5\r\n"
    "host-request-port:%6\r\n"
    "host-wireless-protocol-version:%7\r\n"
    "host-supported-device:PS Vita, PS Vita TV\r\n";

const char *CmaBroadcast::broadcast_query_start = "SRCH";
const char *CmaBroadcast::broadcast_query_end = " * HTTP/1.1\r\n";

const char *CmaBroadcast::broadcast_ok = "HTTP/1.1 200 OK";
const char *CmaBroadcast::broadcast_unavailable = "HTTP/1.1 503 NG";

CmaBroadcast::CmaBroadcast(QObject *obj_parent) :
    QObject(obj_parent)
{
    QSettings settings;
    // generate a GUID if doesn't exist yet in settings
    uuid = settings.value("guid").toString();
    if(uuid.isEmpty()) {
        uuid = QUuid::createUuid().toString().mid(1,36);
        settings.setValue("guid", uuid);
    }

    hostname = settings.value("hostName", QHostInfo::localHostName()).toString();
    setAvailable();

    socket = new QUdpSocket(this);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QHostAddress host_address(QHostAddress::Any);
#else
    QHostAddress host_address(QHostAddress::AnyIPv4);
#endif

    if(!socket->bind(host_address, QCMA_REQUEST_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "Failed to bind address for UDP broadcast";
    }
}

void CmaBroadcast::readPendingDatagrams()
{
    if(socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());

        QHostAddress cma_sender;
        quint16 senderPort;

        socket->readDatagram(datagram.data(), datagram.size(), &cma_sender, &senderPort);

        if(datagram.startsWith(broadcast_query_start) && datagram.contains(broadcast_query_end)) {
            QMutexLocker locker(&mutex);
            socket->writeDatagram(reply, cma_sender, senderPort);
        } else {
            qWarning("Unknown request: %.*s\n", datagram.length(), datagram.constData());
        }
    }
}

void CmaBroadcast::setAvailable()
{
    QMutexLocker locker(&mutex);
    int protocol_version = ::getVitaProtocolVersion();

    reply.clear();    
    reply.insert(0, broadcast_reply
                 .arg(broadcast_ok, uuid, "win", hostname)
                 .arg(protocol_version, 8, 10, QChar('0'))
                 .arg(QCMA_REQUEST_PORT)
                 .arg(VITAMTP_WIRELESS_MAX_VERSION, 8, 10, QChar('0')));
    reply.append('\0');
}

void CmaBroadcast::setUnavailable()
{
    QMutexLocker locker(&mutex);
    int protocol_version = ::getVitaProtocolVersion();

    reply.clear();
    reply.insert(0, broadcast_reply
                 .arg(broadcast_unavailable, uuid, "win", hostname)
                 .arg(protocol_version, 8, 10, QChar('0'))
                 .arg(QCMA_REQUEST_PORT)
                 .arg(VITAMTP_WIRELESS_MAX_VERSION, 8, 10, QChar('0')));
    reply.append('\0');
}
