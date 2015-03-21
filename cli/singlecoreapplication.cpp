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

#include "singlecoreapplication.h"

#include <QDebug>

const int SingleCoreApplication::timeout = 500;
const QString SingleCoreApplication::SHARED_KEY = "QCMA_KEY";

SingleCoreApplication::SingleCoreApplication(int &argc, char **argv) :
    QCoreApplication(argc, argv)
{
    server = new QLocalServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
    QLocalServer::removeServer(SHARED_KEY);
    server->listen(SHARED_KEY);
}

void SingleCoreApplication::receiveMessage()
{
    QLocalSocket *socket = server->nextPendingConnection();

    if(!socket->waitForReadyRead(timeout)) {
        qDebug() << socket->errorString();
        return;
    }

    QByteArray byteArray = socket->readAll();
    QString message = QString::fromUtf8(byteArray.constData());
    emit messageAvailable(message);
    socket->disconnectFromServer();
}

bool SingleCoreApplication::sendMessage(const QString &message)
{
    QLocalSocket socket;
    socket.connectToServer(SHARED_KEY, QIODevice::WriteOnly);

    if(!socket.waitForConnected(timeout)) {
        return false;
    }

    socket.write(message.toUtf8());

    if(!socket.waitForBytesWritten(timeout)) {
        qDebug() << socket.errorString();
        return false;
    }

    socket.disconnectFromServer();
    return true;
}
