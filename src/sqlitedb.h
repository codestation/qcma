/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2014  Codestation
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

#ifndef SQLITEDB_H
#define SQLITEDB_H

#include "database.h"

#include <vitamtp.h>

#include <QFileInfo>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTimer>

class SQLiteDB : public Database
{
    Q_OBJECT
public:
    explicit SQLiteDB(QObject *parent = 0);
    ~SQLiteDB();

    bool load();
    bool rescan();
    void close();

    bool reload(bool &prepared);
    void setUUID(const QString &m_uuid);

    bool open();
    int create();
    void clear();
    bool initialize();
    QSqlError getLastError();

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
    void freeMetadata(metadata_t *metadata);

    int getPathId(const QString &path);
    QString getPathFromId(int ohfi);
    bool updateSize(int ohfi, quint64 size);
    bool deleteEntry(int ohfi);
    bool insertSourceEntry(uint object_id, const QString &path, const QString &name);
    uint insertMusicEntry(const QString &path, const QString &name, int parent, int type);
    uint insertVideoEntry(const QString &path, const QString &name, int parent, int type);
    uint insertPhotoEntry(const QString &path, const QString &name, int parent, int type);
    uint insertSavedataEntry(const QString &path, const QString &name, int parent, int type);
    bool insertApplicationEntry(const QString &name, int ohfi, int app_type);

private:
    int recursiveScanRootDirectory(const QString &base_path, const QString &rel_path, int parent_ohfi, int root_ohfi);
    int insertObjectEntryInternal(const QString &path, const QString &name, int parent_ohfi, int type);
    int insertDefaultEntry(const QString &path, const QString &name, const QString &title, int parent, int type);
    int insertNodeEntry(const QString &title, int type, int data_type);
    bool updateAdjacencyList(int ohfi, int parent);
    QString getBasePath(int root_ohfi);
    int getObjectType(int ohfi);
    void fillMetadata(const QSqlQuery &query, metadata_t &metadata);
    qint64 getChildenTotalSize(int ohfi);
    bool updateObjectPath(int ohfi, const QString &name);
    int getRootItems(int root_ohfi, metadata_t **metadata);
    bool insertVirtualEntries();
    bool insertVirtualEntry(int ohfi);

    QTimer *timer;
    QThread *thread;

    QString m_uuid;
    QSqlDatabase db;
};

#endif // SQLITEDB_H
