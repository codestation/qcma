#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include "database.h"

#include <QObject>
#include <QThread>

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = 0);

    void start();
    void stop();

private:
    int thread_count;
    QMutex mutex;

    Database db;
    QThread *usb_thread;
    QThread *wireless_thread;

signals:
    void deviceConnected(QString);
    void deviceDisconnected();
    void receivedPin(int);
    void stopped();

private slots:
    void refreshDatabase();
    void threadStopped();
};

#endif // CLIENTMANAGER_H
