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
#include "servicemanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <vitamtp.h>

ServiceManager::ServiceManager(QObject *obj_parent) :
    QObject(obj_parent)
{
}

ServiceManager::~ServiceManager()
{
    VitaMTP_Cleanup();
    delete m_db;
}

void ServiceManager::refreshDatabase()
{
    if(m_db->load()) {
        return;
    }

    QTextStream(stdout) << "Database scan has started" << endl;

    if(!m_db->rescan()) {
        qWarning("No PS Vita system has been registered");
    }
}

void ServiceManager::start()
{
    if(VitaMTP_Init() < 0) {
        qCritical("Cannot initialize VitaMTP library");
        return;
    }

    loadDefaultSettings();

    if(QSettings().value("useMemoryStorage", true).toBool()) {
        m_db = new QListDB();
    } else {
        m_db = new SQLiteDB();
    }

    // initializing database for the first use
    refreshDatabase();

    // send a signal when the update is finished
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
        qCritical("You must enable at least USB or Wireless monitoring");
    }
}

void ServiceManager::receiveMessage(QString message)
{
    QTextStream(stdout) << message << endl;
}

void ServiceManager::stop()
{
    if(CmaClient::stop() < 0) {
        QCoreApplication::quit();
    }
}

void ServiceManager::threadStopped()
{
    mutex.lock();
    if(--thread_count == 0) {
        QCoreApplication::quit();
    }
    mutex.unlock();
}

void ServiceManager::loadDefaultSettings()
{
    QString defaultdir;
    QSettings settings;

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    qDebug("photoPath: %s", qPrintable(defaultdir));
    settings.setValue("photoPath", defaultdir);

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    qDebug("musicPath: %s", qPrintable(defaultdir));
    settings.setValue("musicPath", defaultdir);

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    qDebug("photoPath: %s", qPrintable(defaultdir));
    settings.setValue("videoPath", defaultdir);

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PS Vita");
    qDebug("appsPath: %s", qPrintable(defaultdir));
    settings.setValue("appsPath", defaultdir);

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PSV Updates");
    qDebug("urlPath: %s", qPrintable(defaultdir));
    settings.setValue("urlPath", defaultdir);

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PSV Packages");
    qDebug("pkgPath: %s", qPrintable(defaultdir));
    settings.setValue("pkgPath", defaultdir);

    settings.setValue("offlineMode", true);
    settings.setValue("skipMetadata", false);

    // disable USB for now
    settings.setValue("disableUSB", true);

    settings.setValue("disableWireless", false);
    settings.setValue("useMemoryStorage", true);

    settings.setValue("photoSkip", false);
    settings.setValue("videoSkip", false);
    settings.setValue("musicSkip", false);

    settings.setValue("protocolMode", "automatic");

    settings.setValue("protocolIndex", 0);

    settings.setValue("protocolVersion", VITAMTP_PROTOCOL_MAX_VERSION);
}
