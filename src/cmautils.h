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

#ifndef UTILS_H
#define UTILS_H

#include <QByteArray>
#include <QString>
#include <QThread>

#include <vitamtp.h>

// Qt4 doesn't have public methods for Thread::*sleep
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
typedef QThread Sleeper;
#else
class Sleeper : QThread
{
public:
    static void sleep(unsigned long secs) {
        QThread::sleep(secs);
    }
    static void msleep(unsigned long msecs) {
        QThread::msleep(msecs);
    }
    static void usleep(unsigned long usecs) {
        QThread::usleep(usecs);
    }
};
#endif

bool removeRecursively(const QString &path);
QString readable_size(qint64 size, bool use_gib = false);
bool getDiskSpace(const QString &dir, quint64 *free, quint64 *total);
QByteArray getThumbnail(const QString &path, DataType type, metadata_t *metadata);
int getVitaProtocolVersion();

#endif // UTILS_H
