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
    connect(sn_hup, &QSocketNotifier::activated, this, &ClientManager::handleSigHup);
    sn_term = new QSocketNotifier(sigterm_fd[1], QSocketNotifier::Read, this);
    connect(sn_term, &QSocketNotifier::activated, this, &ClientManager::handleSigTerm);
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

    connect(m_db, &Database::fileAdded, &progress, &ProgressForm::setFileName);
    connect(m_db, &Database::directoryAdded, &progress, &ProgressForm::setDirectoryName);
    connect(m_db, &Database::updated, this, &ClientManager::databaseUpdated);
    connect(&progress, &ProgressForm::canceled, m_db, &Database::cancelOperation, Qt::DirectConnection);

    thread_count = 0;
    qDebug("Starting cma threads");
    CmaClient *client;
    QSettings settings;

    if(!settings.value("disableUSB", false).toBool()) {
        usb_thread = new QThread();
        client = new CmaClient(m_db);
        usb_thread->setObjectName("usb_thread");
        connect(usb_thread, &QThread::started, client, &CmaClient::connectUsb);
        connect(client, &CmaClient::messageSent, this, &ClientManager::messageSent);
        connect(client, &CmaClient::finished, usb_thread, &QThread::quit, Qt::DirectConnection);
        connect(usb_thread, &QThread::finished, usb_thread, &QObject::deleteLater);
        connect(usb_thread, &QThread::finished, this, &ClientManager::threadStopped);
        connect(usb_thread, &QThread::finished, client, &QObject::deleteLater);

        connect(client, &CmaClient::deviceConnected, this, &ClientManager::deviceConnected);
        connect(client, &CmaClient::deviceDisconnected, this, &ClientManager::deviceDisconnected);
        connect(client, &CmaClient::refreshDatabase, this, &ClientManager::refreshDatabase);

        client->moveToThread(usb_thread);
        usb_thread->start();
        thread_count++;
    }

    if(!settings.value("disableWireless", false).toBool()) {
        CmaBroadcast *broadcast = new CmaBroadcast(this);
        wireless_thread = new QThread();
        client = new CmaClient(m_db, broadcast);
        wireless_thread->setObjectName("wireless_thread");
        connect(wireless_thread, &QThread::started, client, &CmaClient::connectWireless);
        connect(client, &CmaClient::messageSent, this, &ClientManager::messageSent);
        connect(client, &CmaClient::receivedPin, this, &ClientManager::showPinDialog);
        connect(client, &CmaClient::finished, wireless_thread, &QThread::quit, Qt::DirectConnection);
        connect(wireless_thread, &QThread::finished, wireless_thread, &QObject::deleteLater);
        connect(wireless_thread, &QThread::finished, this, &ClientManager::threadStopped);
        connect(wireless_thread, &QThread::finished, client, &QObject::deleteLater);

        connect(client, &CmaClient::pinComplete, &pinForm, &PinForm::hide);
        connect(client, &CmaClient::deviceConnected, this, &ClientManager::deviceConnected);
        connect(client, &CmaClient::deviceDisconnected, this, &ClientManager::deviceDisconnected);
        connect(client, &CmaClient::refreshDatabase, this, &ClientManager::refreshDatabase);

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
