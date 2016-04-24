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

#include "clientmanager.h"
#include "cmaclient.h"
#include "cmautils.h"
#include "forms/progressform.h"

#include <QSettings>

#include <vitamtp.h>

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <unistd.h>

int ClientManager::sighup_fd[2];
int ClientManager::sigterm_fd[2];
#endif

ClientManager::ClientManager(Database *db, QObject *obj_parent) :
    QObject(obj_parent), m_db(db)
{
#ifdef Q_OS_UNIX
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighup_fd))
       qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigterm_fd))
       qFatal("Couldn't create TERM socketpair");

    sn_hup = new QSocketNotifier(sighup_fd[1], QSocketNotifier::Read, this);
    connect(sn_hup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    sn_term = new QSocketNotifier(sigterm_fd[1], QSocketNotifier::Read, this);
    connect(sn_term, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
#endif
}

ClientManager::~ClientManager()
{
    VitaMTP_Cleanup();
    VitaMTP_USB_Exit();
}

void ClientManager::databaseUpdated(int count)
{
    progress.interruptShow();
    progress.hide();
    if(count >= 0) {
        emit messageSent(tr("Added %1 items to the database").arg(count));
    } else {
        emit messageSent(tr("Database indexing aborted by user"));
    }
    emit updated(count);
}

void ClientManager::showPinDialog(QString name, int pin)
{
    pinForm.setPin(name, pin);
    pinForm.startCountdown();
}

void ClientManager::start()
{
    VitaMTP_Init();

    if(VitaMTP_USB_Init() < 0) {
        emit messageSent(tr("Cannot initialize VitaMTP USB library"));
        return;
    }

    // initializing database for the first use
    refreshDatabase();

    connect(m_db, SIGNAL(fileAdded(QString)), &progress, SLOT(setFileName(QString)));
    connect(m_db, SIGNAL(directoryAdded(QString)), &progress, SLOT(setDirectoryName(QString)));
    connect(m_db, SIGNAL(updated(int)), this, SLOT(databaseUpdated(int)));
    connect(&progress, SIGNAL(canceled()), m_db, SLOT(cancelOperation()), Qt::DirectConnection);

    thread_count = 0;
    qDebug("Starting cma threads");
    CmaClient *client;
    QSettings settings;

    if(!settings.value("disableUSB", false).toBool()) {
        usb_thread = new QThread();
        client = new CmaClient(m_db);
        usb_thread->setObjectName("usb_thread");
        connect(usb_thread, SIGNAL(started()), client, SLOT(connectUsb()));
        connect(client, SIGNAL(messageSent(QString)), this, SIGNAL(messageSent(QString)));
        connect(client, SIGNAL(finished()), usb_thread, SLOT(quit()), Qt::DirectConnection);
        connect(usb_thread, SIGNAL(finished()), usb_thread, SLOT(deleteLater()));
        connect(usb_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
        connect(usb_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

        connect(client, SIGNAL(deviceConnected(QString)), this, SIGNAL(deviceConnected(QString)));
        connect(client, SIGNAL(deviceDisconnected()), this, SIGNAL(deviceDisconnected()));
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
        connect(client, SIGNAL(messageSent(QString)), this, SIGNAL(messageSent(QString)));
        connect(client, SIGNAL(receivedPin(QString,int)), this, SLOT(showPinDialog(QString,int)));
        connect(client, SIGNAL(finished()), wireless_thread, SLOT(quit()), Qt::DirectConnection);
        connect(wireless_thread, SIGNAL(finished()), wireless_thread, SLOT(deleteLater()));
        connect(wireless_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
        connect(wireless_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

        connect(client, SIGNAL(pinComplete()), &pinForm, SLOT(hide()));
        connect(client, SIGNAL(deviceConnected(QString)), this, SIGNAL(deviceConnected(QString)));
        connect(client, SIGNAL(deviceDisconnected()), this, SIGNAL(deviceDisconnected()));
        connect(client, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));

        client->moveToThread(wireless_thread);
        wireless_thread->start();
        thread_count++;
    }

    if(thread_count == 0) {
        emit messageSent(tr("You must enable at least USB or Wireless monitoring"));
    }
}

void ClientManager::refreshDatabase()
{
    if(m_db->load()) {
        return;
    }

    if(!m_db->rescan()) {
        emit messageSent(tr("No PS Vita system has been registered"));
    } else {
        progress.showDelayed(1000);
    }
}

void ClientManager::stop()
{
    if(CmaClient::stop() < 0) {
        emit stopped();
    }
}

void ClientManager::threadStopped()
{
    mutex.lock();
    if(--thread_count == 0) {
        emit stopped();
    }
    mutex.unlock();
}

#ifdef Q_OS_UNIX
void ClientManager::hupSignalHandler(int)
{
    char a = 1;
    ::write(sighup_fd[0], &a, sizeof(a));
}

void ClientManager::termSignalHandler(int)
{
    char a = 1;
    ::write(sigterm_fd[0], &a, sizeof(a));
}

void ClientManager::handleSigTerm()
{
    sn_term->setEnabled(false);
    char tmp;
    ::read(sigterm_fd[1], &tmp, sizeof(tmp));

    stop();

    sn_term->setEnabled(true);
}

void ClientManager::handleSigHup()
{
    sn_hup->setEnabled(false);
    char tmp;
    ::read(sighup_fd[1], &tmp, sizeof(tmp));

    refreshDatabase();

    sn_hup->setEnabled(true);
}
#endif
