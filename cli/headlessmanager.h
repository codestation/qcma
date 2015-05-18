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
#include <QSocketNotifier>

class HeadlessManager : public QObject
{
    Q_OBJECT

public:
    explicit HeadlessManager(QObject *parent = 0);
    ~HeadlessManager();

    void start();

    // unix signal handlers
    static void hupSignalHandler(int);
    static void termSignalHandler(int);

private:
    int thread_count;
    QMutex mutex;

    Database *m_db;

    QThread *usb_thread;
    QThread *wireless_thread;

    // signal handling
    static int sighup_fd[2];
    static int sigterm_fd[2];

    QSocketNotifier *sn_hup;
    QSocketNotifier *sn_term;

signals:
    void stopped();
    void databaseUpdated(int count);
    void messageSent(QString);

public slots:
    void refreshDatabase();
    void stop();

    // Qt signal handlers
    void handleSigHup();
    void handleSigTerm();

private slots:
    void threadStopped();
    void receiveMessage(QString message);
};

#endif // HEADLESSMANAGER_H
