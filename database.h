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

#ifndef DATABASE_H
#define DATABASE_H

#include "cmarootobject.h"

#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>

extern "C" {
#include <vitamtp.h>
}

class Database : public QObject
{
    Q_OBJECT
public:
    typedef struct {
        QList<CMAObject *>::const_iterator it;
        QList<CMAObject *>::const_iterator end;
    } find_data;

    explicit Database(QObject *parent = 0);
    ~Database();


    void destroy();
    void setUUID(const QString uuid);
    int create();
    void addEntries(CMAObject *root);
    CMAObject *ohfiToObject(int ohfi);
    bool find(int ohfi, find_data &data);
    void append(int parent_ohfi, CMAObject *object);
    bool remove(const CMAObject *obj, int ohfi_root = 0);
    int filterObjects(int ohfiParent, metadata_t **p_head);
    CMAObject *pathToObject(const char *path, int ohfiRoot);
    int acceptFilteredObject(const CMAObject *parent, const CMAObject *current, int type);

    QMutex mutex;

private:
    typedef QList<CMAObject *> root_list;
    typedef QMap<int, root_list> map_list;

    static const QStringList audio_types;
    static const QStringList image_types;
    static const QStringList video_types;

    int scanRootDirectory(root_list &list,int ohfi_type);
    int recursiveScanRootDirectory(root_list &list, CMAObject *parent, int ohfi_type);
    bool hasFilter(const CMARootObject *object,int ohfi);
    bool removeInternal(root_list &list, const CMAObject *obj);
    bool findInternal(const root_list &list, int ohfi, find_data &data);
    CMAObject *getParent(CMAObject *last_dir, const QString &current_path);
    CMAObject *pathToObjectInternal(const root_list &list, const char *path);
    static bool lessThanComparator(const CMAObject *a, const CMAObject *b);
    bool checkFileType(const QString path, int ohfi_root);
    void dumpMetadataList(const metadata_t *p_head);

    map_list object_list;    

signals:
    void finished();
};

#endif // DATABASE_H
