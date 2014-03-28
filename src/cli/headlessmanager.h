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

#ifndef HEADLESSMANAGER_H
#define HEADLESSMANAGER_H

#include "database.h"

#include <QObject>
#include <QThread>
#include <QDBusConnection>

class HeadlessManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qcma.HeadlessManager")

public:
    explicit HeadlessManager(QObject *parent = 0);
    ~HeadlessManager();

    void start();

private:
    int thread_count;
    QMutex mutex;

    Database *m_db;

    QThread *usb_thread;
    QThread *wireless_thread;
    QDBusConnection dbus_conn;

signals:
    void stopped();
    Q_SCRIPTABLE void databaseUpdated(int count);

public slots:
    void refreshDatabase();
    void stop();

private slots:
    void threadStopped();
};

#endif // HEADLESSMANAGER_H
