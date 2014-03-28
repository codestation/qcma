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

#ifndef SINGLECOREAPPLICATION_H
#define SINGLECOREAPPLICATION_H

#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSharedMemory>

class SingleCoreApplication : public QCoreApplication
{
    Q_OBJECT
public:
    explicit SingleCoreApplication(int &argc, char **argv);

    static bool sendMessage(const QString &message);

private:
    QLocalServer *server;

    static const int timeout;
    static const QString SHARED_KEY;

signals:
    void messageAvailable(QString message);


public slots:
    void receiveMessage();
};

#endif // SINGLECOREAPPLICATION_H
