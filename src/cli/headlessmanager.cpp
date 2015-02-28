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
#include "cmautils.h"
#include "sqlitedb.h"
#include "qlistdb.h"
#include "headlessmanager.h"
#include "headlessmanager_adaptor.h"

#include <QCoreApplication>
#include <QSettings>
#include <vitamtp.h>

HeadlessManager::HeadlessManager(QObject *obj_parent) :
    QObject(obj_parent), dbus_conn(QDBusConnection::sessionBus())
{
    new HeadlessManagerAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    // expose qcma over dbus so the database update can be triggered
    dbus.registerObject("/HeadlessManager", this);
    dbus.registerService("org.qcma.HeadlessManager");
}

HeadlessManager::~HeadlessManager()
{
    VitaMTP_Cleanup();
    delete m_db;
}

void HeadlessManager::refreshDatabase()
{
    if(m_db->load()) {
        return;
    }

    if(!m_db->rescan()) {
        qDebug("No PS Vita system has been registered");
    }
}

void HeadlessManager::start()
{
    if(VitaMTP_Init() < 0) {
        qDebug("Cannot initialize VitaMTP library");
        return;
    }

    if(QSettings().value("useMemoryStorage", true).toBool()) {
        m_db = new QListDB();
    } else {
        m_db = new SQLiteDB();
    }

    // initializing database for the first use
    refreshDatabase();

    // send a signal over dbus of the connected peers knows when the update is finished
    connect(m_db, SIGNAL(updated(int)), this, SIGNAL(databaseUpdated(int)));

    thread_count = 0;
    qDebug("Starting cma threads");
    CmaClient *client;
    QSettings settings;

    if(!settings.value("disableUSB", false).toBool()) {
        usb_thread = new QThread();
        client = new CmaClient(m_db);
        usb_thread->setObjectName("usb_thread");
        connect(usb_thread, SIGNAL(started()), client, SLOT(connectUsb()));
        connect(client, SIGNAL(finished()), usb_thread, SLOT(quit()), Qt::DirectConnection);
        connect(usb_thread, SIGNAL(finished()), usb_thread, SLOT(deleteLater()));
        connect(usb_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
        connect(usb_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

        connect(client, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));

        client->moveToThread(usb_thread);
        usb_thread->start();
        thread_count++;
    }

    if(!settings.value("disableWireless", false).toBool()) {
        CmaBroadcast *broadcast = new CmaBroadcast(this);
        wireless_thread = new QThread();
        client = new CmaClient(m_db, broadcast);
        wireless_thread->setObjectName("wireless_thread");
        connect(wireless_thread, SIGNAL(started()), client, SLOT(connectWireless()));
        connect(client, SIGNAL(finished()), wireless_thread, SLOT(quit()), Qt::DirectConnection);
        connect(wireless_thread, SIGNAL(finished()), wireless_thread, SLOT(deleteLater()));
        connect(wireless_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
        connect(wireless_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

        connect(client, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));

        client->moveToThread(wireless_thread);
        wireless_thread->start();
        thread_count++;
    }

    if(thread_count == 0) {
        qDebug("You must enable at least USB or Wireless monitoring");
    }
}

void HeadlessManager::stop()
{
    if(CmaClient::stop() < 0) {
        QCoreApplication::quit();
    }
}

void HeadlessManager::threadStopped()
{
    mutex.lock();
    if(--thread_count == 0) {
        QCoreApplication::quit();
    }
    mutex.unlock();
}
