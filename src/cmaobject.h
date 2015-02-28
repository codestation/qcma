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

#ifndef CMAOBJECT_H
#define CMAOBJECT_H

#include <QFileInfo>
#include <QString>

#include <vitamtp.h>

#define OHFI_OFFSET 256

class CMAObject
{
public:
    explicit CMAObject(CMAObject *parent = 0);
    ~CMAObject();

    void refreshPath();
    void rename(const QString &name);
    void updateObjectSize(qint64 size);
    bool hasParent(const CMAObject *obj);
    void initObject(const QFileInfo &file, int file_type = -1);

    bool operator==(const CMAObject &obj);
    bool operator!=(const CMAObject &obj);
    bool operator<(const CMAObject &obj);

    inline void setOhfi(int ohfi) {
        metadata.ohfi = ohfi;
    }

    inline static void resetOhfiCounter() {
        ohfi_count = OHFI_OFFSET;
    }

    QString m_path;
    CMAObject *parent;
    metadata_t metadata;

protected:
    static int ohfi_count;

private:
    void loadSfoMetadata(const QString &path);
};

#endif // CMAOBJECT_H
