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

#include "utils.h"

#include <QDir>
#include <QImage>

extern "C" {
#include <vitamtp.h>
}

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

bool getDiskSpace(const QString &dir, quint64 *free, quint64 *total)
{
#ifdef Q_OS_WIN32

    if(GetDiskFreeSpaceExW(dir.toStdWString().c_str(), free, total, NULL) != 0) {
        return true;
    }

#else
    struct statvfs64 stat;

    if(statvfs64(dir.toUtf8().data(), &stat) == 0) {
        *total = stat.f_frsize * stat.f_blocks;
        *free = stat.f_frsize * stat.f_bfree;
        return true;
    }

#endif
    return false;
}

bool removeRecursively(const QString &dirName)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QDir(dirName).removeRecursively();
#else
    bool result = false;
    QDir dir(dirName);

    if(dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if(info.isDir()) {
                result = removeRecursively(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if(!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
#endif
}
