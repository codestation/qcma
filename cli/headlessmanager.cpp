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

#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include <sys/socket.h>
#include <unistd.h>
#include <vitamtp.h>

int HeadlessManager::sighup_fd[2];
int HeadlessManager::sigterm_fd[2];

HeadlessManager::HeadlessManager(QObject *obj_parent) :
    QObject(obj_parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighup_fd))
       qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigterm_fd))
       qFatal("Couldn't create TERM socketpair");

    sn_hup = new QSocketNotifier(sighup_fd[1], QSocketNotifier::Read, this);
    connect(sn_hup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    sn_term = new QSocketNotifier(sigterm_fd[1], QSocketNotifier::Read, this);
    connect(sn_term, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
}

HeadlessManager::~HeadlessManager()
{
    VitaMTP_Cleanup();
    VitaMTP_USB_Exit();
    delete m_db;
}

void HeadlessManager::refreshDatabase()
{
    if(m_db->load()) {
        return;
    }

    QTextStream(stdout) << "Database scan has started" << endl;

    if(!m_db->rescan()) {
        qWarning("No PS Vita system has been registered");
    }
}

void HeadlessManager::start()
{
    VitaMTP_Init();

    if(VitaMTP_USB_Init() < 0) {
        emit messageSent(tr("Cannot initialize VitaMTP USB library"));
        return;
    }

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

void HeadlessManager::receiveMessage(QString message)
{
    QTextStream(stdout) << message << endl;
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

void HeadlessManager::hupSignalHandler(int)
{
    char a = 1;
    ::write(sighup_fd[0], &a, sizeof(a));
}

void HeadlessManager::termSignalHandler(int)
{
    char a = 1;
    ::write(sigterm_fd[0], &a, sizeof(a));
}

void HeadlessManager::handleSigTerm()
{
    sn_term->setEnabled(false);
    char tmp;
    ::read(sigterm_fd[1], &tmp, sizeof(tmp));

    stop();

    sn_term->setEnabled(true);
}

void HeadlessManager::handleSigHup()
{
    sn_hup->setEnabled(false);
    char tmp;
    ::read(sighup_fd[1], &tmp, sizeof(tmp));

    refreshDatabase();

    sn_hup->setEnabled(true);
}
