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

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include "database.h"

#include <QObject>
#include <QThread>

class ServiceManager : public QObject
{
    Q_OBJECT

public:
    explicit ServiceManager(QObject *parent = 0);
    ~ServiceManager();

    void start();

private:
    void loadDefaultSettings();

    int thread_count;
    QMutex mutex;

    Database *m_db;

    QThread *usb_thread;
    QThread *wireless_thread;

signals:
    void stopped();
    void databaseUpdated(int count);

public slots:
    void refreshDatabase();
    void stop();

private slots:
    void threadStopped();
    void receiveMessage(QString message);
};

#endif // SERVICEMANAGER_H
