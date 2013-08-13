#ifndef CMASERVER_H
#define CMASERVER_H

#include "wirelessworker.h"

extern "C" {
#include <vitamtp.h>
}

#include <limits.h>

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QWaitCondition>

class CmaServer : public QObject
{
    Q_OBJECT
public:
    explicit CmaServer(QObject *parent = 0);
    void listen();
    void close();

    int num_retries;
    QTimer timer;
    QThread *thread;
    char hostname[256];
    BroadcastSignal broadcast;

signals:
    void newConnection(vita_device_t *);
    void createdPin(int);
    void finished();

public slots:
    void continueProcess();
    void stopProcess();

private slots:
    void connectWireless();
    void connectUsb();
};

#endif // CMASERVER_H
