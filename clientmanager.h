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

#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include "pinform.h"
#include "database.h"
#include "progressform.h"

#include <QObject>
#include <QThread>

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = 0);
    ~ClientManager();

    void start();
    void stop();

    Database db;

private:
    int thread_count;
    QMutex mutex;

    PinForm pinForm;
    ProgressForm progress;

    QThread *usb_thread;
    QThread *wireless_thread;

signals:
    void stopped();
    void receivedPin(int);
    void deviceDisconnected();
    void messageSent(QString);
    void deviceConnected(QString);

private slots:
    void threadStopped();
    void refreshDatabase();
    void databaseUpdated(int count);
    void showPinDialog(QString name, int pin);
};

#endif // CLIENTMANAGER_H
