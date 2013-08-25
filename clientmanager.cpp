#include "clientmanager.h"

#include "cmaclient.h"

ClientManager::ClientManager(QObject *parent) :
    QObject(parent)
{
}

void ClientManager::start()
{
    // initializing database for the first use
    refreshDatabase();
    CmaEvent::db = &db;

    thread_count = 2;
    qDebug("Starting cma threads");
    CmaClient *client;

    usb_thread = new QThread();
    client = new CmaClient();
    usb_thread->setObjectName("usb_thread");
    connect(usb_thread, SIGNAL(started()), client, SLOT(connectUsb()));
    connect(client, SIGNAL(receivedPin(int)), this, SIGNAL(receivedPin(int)));
    connect(client, SIGNAL(finished()), usb_thread, SLOT(quit()), Qt::DirectConnection);
    connect(usb_thread, SIGNAL(finished()), usb_thread, SLOT(deleteLater()));
    connect(usb_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
    connect(usb_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

    connect(client, SIGNAL(deviceConnected(QString)), this, SIGNAL(deviceConnected(QString)));
    connect(client, SIGNAL(deviceDisconnected()), this, SIGNAL(deviceDisconnected()));
    connect(client, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));

    client->moveToThread(usb_thread);
    usb_thread->start();

    wireless_thread = new QThread();
    client = new CmaClient();
    wireless_thread->setObjectName("wireless_thread");
    connect(wireless_thread, SIGNAL(started()), client, SLOT(connectWireless()));
    connect(client, SIGNAL(finished()), wireless_thread, SLOT(quit()), Qt::DirectConnection);
    connect(wireless_thread, SIGNAL(finished()), wireless_thread, SLOT(deleteLater()));
    connect(wireless_thread, SIGNAL(finished()), this, SLOT(threadStopped()));
    connect(wireless_thread, SIGNAL(finished()), client, SLOT(deleteLater()));

    connect(client, SIGNAL(deviceConnected(QString)), this, SIGNAL(deviceConnected(QString)));
    connect(client, SIGNAL(deviceDisconnected()), this, SIGNAL(deviceDisconnected()));
    connect(client, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));

    client->moveToThread(wireless_thread);
    wireless_thread->start();
}

void ClientManager::refreshDatabase()
{
    QMutexLocker locker(&db.mutex);

    db.destroy();
    int count = db.create();
    qDebug("Added %i entries to the database", count);
    emit databaseUpdated(tr("Added %1 entries to the database").arg(count));
}

void ClientManager::stop()
{
    CmaClient::stop();
}

void ClientManager::threadStopped()
{
    mutex.lock();
    if(--thread_count == 0) {
        emit stopped();
    }
    mutex.unlock();
}
