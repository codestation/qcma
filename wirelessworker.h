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

#ifndef WIRELESSWORKER_H
#define WIRELESSWORKER_H

#include "baseworker.h"

#include <QObject>
#include <QSemaphore>

extern "C" {
#include <vitamtp.h>
}

class BroadcastSignal : public BaseWorker
{
    Q_OBJECT
public:
    explicit BroadcastSignal(QObject *parent = 0);
    ~BroadcastSignal();

    static int deviceRegistered(const char *deviceid);
    static int generatePin(wireless_vita_info_t *info, int *p_err);
    bool isRunning() { return started; }

    static wireless_host_info_t info;


private:
    volatile bool started;
    //used to emit a signal from a static method
    static BroadcastSignal *this_object;

signals:
    void createdPin(int);

public slots:
    void stopBroadcast();

protected slots:
    void process();
};

#endif // WIRELESSWORKER_H
