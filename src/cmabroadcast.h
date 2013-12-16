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

#ifndef CMABROADCAST_H
#define CMABROADCAST_H

#include <QMutex>
#include <QObject>
#include <QUdpSocket>

#define QCMA_REQUEST_PORT 9309

class CmaBroadcast : public QObject
{
    Q_OBJECT
public:
    explicit CmaBroadcast(QObject *parent = 0);

private:
    void replyBroadcast(const QByteArray &datagram);

    QMutex mutex;
    QString uuid;
    QByteArray reply;
    QString hostname;
    QUdpSocket *socket;

    static const QString broadcast_reply;
    static const char *broadcast_query_start;
    static const char *broadcast_query_end;
    static const char *broadcast_ok;
    static const char *broadcast_unavailable;

public slots:
    void setAvailable();
    void setUnavailable();

private slots:
    void readPendingDatagrams();
};

#endif // CMABROADCAST_H
