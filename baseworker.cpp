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

void BaseWorker::onFinished()
{
}

void BaseWorker::start(const char *thread_name)
{
    thread = new QThread();

    if(thread_name) {
        thread->setObjectName(thread_name);
    }

    // Move this service to a new thread
    this->moveToThread(thread);

    // The main loop will be executed when the thread
    // signals that it has started.
    connect(thread, SIGNAL(started()), this, SLOT(process()));

    // Make sure that we notify ourselves when the thread
    // is finished in order to correctly clean-up the thread.
    connect(thread, SIGNAL(finished()), this, SLOT(onFinished()));

    // The thread will quit when the sercives
    // signals that it's finished.
    connect(this, SIGNAL(finished()), thread, SLOT(quit()));

    // The thread will be scheduled for deletion when the
    // service signals that it's finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    // Start the thread
    thread->start();
}
