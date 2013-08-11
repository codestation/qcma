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

#ifndef QCMA_H
#define QCMA_H

#include "baseworker.h"

#include <QSystemTrayIcon>
#include <QThread>
#include <QWidget>
#include <QMenu>
#include <QWaitCondition>
#include <QSemaphore>

class QMenu;

extern "C" {
#include <vitamtp.h>
}

#define QCMA_VERSION_STRING "QCMA 0.1"

class QCMA : public BaseWorker
{
    Q_OBJECT
public:
    explicit QCMA(QObject *parent = 0);

private:
    QSemaphore sema;
    QWaitCondition waitCondition;
    volatile bool break_loop;
    volatile bool exit_thread;
    vita_device_t *connectUsb();
    vita_device_t *connectWireless();

signals:
    void createdPin(int);
    void deviceConnected(QString);
    void statusUpdated(QString);
    void deviceDisconnected();
    void stopCMA();

public slots:
    void stop();
    void reload();

protected slots:
    void allowRefresh();
    virtual void process();
};

#endif // QCMA_H
