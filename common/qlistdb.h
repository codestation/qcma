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

#ifndef QLISTDB_H
#define QLISTDB_H

#include "database.h"
#include "cmarootobject.h"

#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>

#include <vitamtp.h>

class QListDB : public Database
{
    Q_OBJECT
public:
    explicit QListDB(QObject *parent = 0);
    ~QListDB();

    bool load();
    bool rescan();
    void close();
    void clear();

    bool reload(bool &prepared);
    void setUUID(const QString &uuid);

    int childObjectCount(int parent_ohfi);
    bool deleteEntry(int ohfi, int root_ohfi = 0);
    QString getAbsolutePath(int ohfi);
    bool getObjectList(int ohfi, metadata_t **metadata);
    bool getObjectMetadata(int ohfi, metadata_t &metadata);
    int getObjectMetadatas(int parent_ohfi, metadata_t **metadata, int index = 0, int max_number = 0);
    qint64 getObjectSize(int ohfi);
    int getParentId(int ohfi);
    int getPathId(const char *name, int ohfi);
    QString getRelativePath(int ohfi);
    int getRootId(int ohfi);
    int insertObjectEntry(const QString &path, const QString &name, int parent_ohfi);
    bool renameObject(int ohfi, const QString &name);
    void setObjectSize(int ohfi, qint64 size);
    void freeMetadata(metadata_t *metadata) {
        Q_UNUSED(metadata);
    }

private:
    typedef struct {
        QList<CMAObject *>::const_iterator it;
        QList<CMAObject *>::const_iterator end;
    } find_data;

    typedef QList<CMAObject *> root_list;
    typedef QMap<int, root_list> map_list;

    int create();
    int createFromOhfi(int ohfi);
    int scanRootDirectory(root_list &list,int ohfi_type);
    int recursiveScanRootDirectory(root_list &list, CMAObject *parent, int ohfi_type);
    bool hasFilter(const CMARootObject *object,int ohfi);
    bool removeInternal(root_list &list, int ohfi);
    bool findInternal(const root_list &list, int ohfi, find_data &data);
    CMAObject *getParent(CMAObject *last_dir, const QString &current_path);
    CMAObject *pathToObjectInternal(const root_list &list, const char *path);
    static bool lessThanComparator(const CMAObject *a, const CMAObject *b);
    void dumpMetadataList(const metadata_t *p_head);
    bool find(int ohfi, find_data &data);
    int acceptFilteredObject(const CMAObject *parent, const CMAObject *current, int type);
    CMAObject *ohfiToObject(int ohfi);

    QTimer *timer;
    QThread *thread;
    map_list object_list;
};

#endif // QLISTDB_H
