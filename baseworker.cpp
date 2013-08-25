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

#include "baseworker.h"

BaseWorker::BaseWorker(QObject *parent) :
    QObject(parent)
{
    thread = NULL;
}

void BaseWorker::start(const char *thread_name)
{
    thread = new QThread();

    if(thread_name) {
        thread->setObjectName(thread_name);
    }

    this->moveToThread(thread);

    connect(thread, SIGNAL(started()), this, SLOT(process()));
    connect(this, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(this, SIGNAL(finished()), thread, SLOT(quit()), Qt::DirectConnection);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}

bool BaseWorker::wait()
{
    return thread->wait();
}

void BaseWorker::onFinished()
{
}
