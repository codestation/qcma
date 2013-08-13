#include "cmaserver.h"
#include "wirelessworker.h"

#include <QDebug>
#include <QHostInfo>
#include <QMutex>
#include <QSettings>

CmaServer::CmaServer(QObject *parent) :
    QObject(parent), num_retries(0)
{
    strcpy(hostname, QHostInfo::localHostName().toStdString().c_str());
    BroadcastSignal::info.name = hostname;
}

void CmaServer::listen()
{
    QThread *thread = new QThread();
    if(QSettings().value("wireless", false).toBool()) {
        connect(&timer, SIGNAL(timeout()), this, SLOT(connectWireless()));
        broadcast.start();
        timer.setSingleShot(true);
    } else {
        timer.setSingleShot(false);
        connect(&timer, SIGNAL(timeout()), this, SLOT(connectUsb()));
    }
    moveToThread(thread);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    timer.start(2000);
    thread->start();
}

void CmaServer::connectUsb()
{
    qDebug() << "From connectUsb: "<< QThread::currentThreadId();

    vita_device_t *vita = VitaMTP_Get_First_USB_Vita();
    if(vita) {
        qDebug("Detected PS Vita via USB");
        timer.stop();
        emit newConnection(vita);
    } else {
        qDebug("No PS Vita found. Attempt %d", ++num_retries);
    }
}

void CmaServer::connectWireless()
{
    typedef BroadcastSignal BS;

    qDebug() << "From connectWireless: "<< QThread::currentThreadId();

    vita_device_t *vita = VitaMTP_Get_First_Wireless_Vita(&BS::info, 0, 0, BS::deviceRegistered, BS::generatePin);
    if(vita) {
        qDebug("Detected PS Vita in wireless mode");
        emit newConnection(vita);
    } else {
        qDebug("PS Vita couldn't be detected. Attempt %d", ++num_retries);
    }
}

void CmaServer::continueProcess()
{
    qDebug("Restarting CmaServer thread");
    num_retries = 0;
    timer.start();
}

void CmaServer::stopProcess()
{
    timer.stop();
}
