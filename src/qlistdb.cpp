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

#include "cmautils.h"
#include "qlistdb.h"
#include "cmaobject.h"

#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QTextStream>
#include <QThread>
#include <QDebug>

QListDB::QListDB(QObject *parent) :
    Database(parent)
{
    QString uuid = QSettings().value("lastAccountId", "ffffffffffffffff").toString();
    CMARootObject::uuid  = uuid;
    thread = new QThread();
    moveToThread(thread);
    timer = new QTimer();
    thread->start();

    timer->setInterval(0);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(process()));
}

QListDB::~QListDB()
{
    clear();
    timer->stop();
    delete timer;
    thread->quit();
    thread->wait();
    delete thread;
}

void QListDB::setUUID(const QString &uuid)
{
    CMARootObject::uuid = uuid;
    QSettings().setValue("lastAccountId", uuid);
}

bool QListDB::load()
{
    // not implemented
    return false;
}

bool QListDB::rescan()
{
    if(mutex.tryLock(1000)) {
        if(CMARootObject::uuid != "ffffffffffffffff") {
            timer->start();
            return true;
        } else {
            mutex.unlock();
            return false;
        }
    }
    return false;
}

void QListDB::clear()
{
    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        CMARootObject *first = static_cast<CMARootObject *>((*root).takeFirst());
        delete first;
        qDeleteAll(*root);
    }

    object_list.clear();
}

int QListDB::create()
{
    int total_objects = 0;
    //QMutexLocker locker(&mutex);
    const int ohfi_array[] = { VITA_OHFI_MUSIC, VITA_OHFI_PHOTO, VITA_OHFI_VIDEO,
                               VITA_OHFI_BACKUP, VITA_OHFI_VITAAPP, VITA_OHFI_PSPAPP,
                               VITA_OHFI_PSPSAVE, VITA_OHFI_PSXAPP, VITA_OHFI_PSMAPP
                             };
    CMAObject::resetOhfiCounter();
    QSettings settings;

    for(int i = 0, max = sizeof(ohfi_array) / sizeof(int); i < max; i++) {
        CMARootObject *obj = new CMARootObject(ohfi_array[i]);
        bool skipCurrent = false;
        int dir_count;

        switch(ohfi_array[i]) {
        case VITA_OHFI_MUSIC:
            obj->initObject(settings.value("musicPath").toString());
            skipCurrent = settings.value("musicSkip", false).toBool();
            break;

        case VITA_OHFI_PHOTO:
            obj->initObject(settings.value("photoPath").toString());
            skipCurrent = settings.value("photoSkip", false).toBool();
            break;

        case VITA_OHFI_VIDEO:
            obj->initObject(settings.value("videoPath").toString());
            skipCurrent = settings.value("videoSkip", false).toBool();
            break;

        case VITA_OHFI_BACKUP:
        case VITA_OHFI_VITAAPP:
        case VITA_OHFI_PSPAPP:
        case VITA_OHFI_PSPSAVE:
        case VITA_OHFI_PSXAPP:
        case VITA_OHFI_PSMAPP:
            obj->initObject(settings.value("appsPath").toString());
        }

        root_list list;
        list << obj;
        emit directoryAdded(obj->path);

        if(!skipCurrent) {
            dir_count = recursiveScanRootDirectory(list, obj, ohfi_array[i]);
        } else {
            dir_count = 0;
        }

        if(dir_count < 0) {
            return -1;
        }

        qDebug("Added objects for OHFI 0x%02X: %i", ohfi_array[i], dir_count);

        total_objects += dir_count;
        object_list[ohfi_array[i]] = list;
    }
    return total_objects;
}

CMAObject *QListDB::getParent(CMAObject *last_dir, const QString &current_path)
{
    while(last_dir && current_path != last_dir->path) {
        last_dir = last_dir->parent;
    }

    return last_dir;
}

int QListDB::scanRootDirectory(root_list &list, int ohfi_type)
{
    int file_type = -1;
    int total_objects = 0;
    CMAObject *last_dir = list.first();
    QDir dir(last_dir->path);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QDirIterator it(dir, QDirIterator::Subdirectories);

    while(it.hasNext()) {

        if(!continueOperation()) {
            return -1;
        }

        it.next();
        QFileInfo info = it.fileInfo();

        if(info.isFile()) {
            if((file_type = checkFileType(info.absoluteFilePath(), ohfi_type)) < 0) {
                //qDebug("Excluding %s from database", info.absoluteFilePath().toStdString().c_str());
                continue;
            }
        }

        CMAObject *obj = new CMAObject(getParent(last_dir, info.path()));
        obj->initObject(info, file_type);
        //qDebug("Added %s to database with OHFI %d", obj->metadata.name, obj->metadata.ohfi);
        list << obj;

        if(obj->metadata.dataType & Folder) {
            last_dir = obj;
        } else {
            total_objects++;
        }
    }
    return total_objects;
}

int QListDB::recursiveScanRootDirectory(root_list &list, CMAObject *parent, int ohfi_type)
{
    int file_type = -1;
    int total_objects = 0;

    QDir dir(parent->path);
    dir.setSorting(QDir::Name | QDir::DirsFirst);
    QFileInfoList qsl = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &info, qsl) {

        if(!continueOperation()) {
            return -1;
        }

        if(info.isFile() && (file_type = checkFileType(info.absoluteFilePath(), ohfi_type)) < 0) {
            //qDebug("Excluding %s from database", info.absoluteFilePath().toStdString().c_str());>
        } else {
            CMAObject *obj = new CMAObject(parent);
            obj->initObject(info, file_type);
            emit fileAdded(obj->metadata.name);
            //qDebug("Added %s to database with OHFI %d", obj->metadata.name, obj->metadata.ohfi);
            list << obj;
            if(info.isDir()) {
                emit directoryAdded(obj->path);
                total_objects += recursiveScanRootDirectory(list, obj, ohfi_type);
            } else {
                total_objects++;
            }
        }
    }

    return total_objects;
}

bool QListDB::removeInternal(root_list &list, int ohfi)
{
    bool found = false;
    QList<CMAObject *>::iterator it = list.begin();

    while(it != list.end()) {
        if(!found && (*it)->metadata.ohfi == ohfi) {
            // update the size of the parent objects
            (*it)->updateObjectSize(-(*it)->metadata.size);
            it = list.erase(it);
            found = true;
        } else if(found && (*it)->metadata.ohfiParent == ohfi) {
            it = list.erase(it);
        } else {
            ++it;
        }
    }

    return found;
}

bool QListDB::lessThanComparator(const CMAObject *a, const CMAObject *b)
{
    return a->metadata.ohfi < b->metadata.ohfi;
}

bool QListDB::hasFilter(const CMARootObject *object,int ohfi)
{
    for(int i = 0; i < object->num_filters; i++) {
        if(object->filters[i].ohfi == ohfi) {
            return true;
        }
    }
    return false;
}

bool QListDB::findInternal(const root_list &list, int ohfi, find_data &data)
{
    if(hasFilter(static_cast<CMARootObject *>(list.first()), ohfi)) {
        data.it = list.begin();
    } else {
        CMAObject obj;
        obj.setOhfi(ohfi);
        data.it = qBinaryFind(list.begin(), list.end(), &obj, QListDB::lessThanComparator);
    }
    data.end = list.end();
    return data.it != data.end;
}

bool QListDB::find(int ohfi, QListDB::find_data &data)
{
    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        if(findInternal(*root, ohfi, data)) {
            return true;
        }
    }
    return false;
}

CMAObject *QListDB::ohfiToObject(int ohfi)
{
    find_data data;
    return find(ohfi, data) ? *data.it : NULL;
}

CMAObject *QListDB::pathToObjectInternal(const root_list &list, const char *path)
{
    // skip the first element since is the root element
    root_list::const_iterator skipped_first = ++list.begin();

    for(root_list::const_iterator obj = skipped_first; obj != list.end(); ++obj) {
        if(strcasecmp(path, (*obj)->metadata.path) == 0) {
            return (*obj);
        }
    }
    return NULL;
}

int QListDB::acceptFilteredObject(const CMAObject *parent, const CMAObject *current, int type)
{
    QMutexLocker locker(&mutex);
    int result = 0;

    if(MASK_SET(type, VITA_DIR_TYPE_MASK_PHOTO)) {
        result = (current->metadata.dataType & Photo);
    } else if(MASK_SET(type, VITA_DIR_TYPE_MASK_VIDEO)) {
        result = (current->metadata.dataType & Video);
    } else if(MASK_SET(type, VITA_DIR_TYPE_MASK_MUSIC)) {
        result = (current->metadata.dataType & Music);
    }

    if(type == (VITA_DIR_TYPE_MASK_MUSIC | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_ARTISTS)) {
        // unimplemented
        return 0;
    } else if(type == (VITA_DIR_TYPE_MASK_MUSIC | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_GENRES)) {
        // unimplemented
        return 0;
    } else if(type == (VITA_DIR_TYPE_MASK_MUSIC | VITA_DIR_TYPE_MASK_ROOT | VITA_DIR_TYPE_MASK_PLAYLISTS)) {
        // unimplemented
        return 0;
    } else if(type & (VITA_DIR_TYPE_MASK_ALL | VITA_DIR_TYPE_MASK_SONGS)) {
        result = result && (current->metadata.dataType & File);
    } else if(type & (VITA_DIR_TYPE_MASK_REGULAR)) {
        result = (parent->metadata.ohfi == current->metadata.ohfiParent);
    }

    // TODO: Support other filter types
    return result;
}

void QListDB::dumpMetadataList(const metadata_t *p_head)
{
    QMutexLocker locker(&mutex);

    while(p_head) {
        qDebug("Metadata: %s with OHFI %d", p_head->name, p_head->ohfi);
        p_head = p_head->next_metadata;
    }
}

bool QListDB::getObjectMetadata(int ohfi, metadata_t &metadata)
{
    QMutexLocker locker(&mutex);

    CMAObject *obj = ohfiToObject(ohfi);
    if(obj) {
        //TODO: return the pointer instead of copying the struct
        metadata = obj->metadata;
        return true;
    }
    return false;
}

int QListDB::childObjectCount(int parent_ohfi)
{
    return getObjectMetadatas(parent_ohfi, NULL);
}

bool QListDB::deleteEntry(int ohfi, int root_ohfi)
{
    QMutexLocker locker(&mutex);

    if(root_ohfi) {
        return removeInternal(object_list[root_ohfi], ohfi);
    } else {
        for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
            if(removeInternal(*root, ohfi)) {
                return true;
            }
        }
    }
    return false;
}

int QListDB::getObjectMetadatas(int parent_ohfi, metadata_t **metadata, int index, int max_number)
{
    QMutexLocker locker(&mutex);

    CMARootObject *parent = static_cast<CMARootObject *>(ohfiToObject(parent_ohfi));

    if(parent == NULL) {
        return 0;
    }

    if(parent->metadata.dataType & File) {
        *metadata = &parent->metadata;
        return 1;
    }

    int type = parent->metadata.type;

    if(parent->metadata.ohfi < OHFI_OFFSET && parent->filters) { // if we have filters
        if(parent_ohfi == parent->metadata.ohfi) { // if we are looking at root
            return parent->getFilters(metadata);
        } else { // we are looking at a filter
            for(int j = 0; j < parent->num_filters; j++) {
                if(parent->filters[j].ohfi == parent_ohfi) {
                    type = parent->filters[j].type;
                    break;
                }
            }
        }
    }

    int offset = 0;
    int numObjects = 0;
    metadata_t temp = metadata_t();
    metadata_t *tail = &temp;

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        for(root_list::iterator object = (*root).begin(); object != (*root).end(); ++object) {
            if(acceptFilteredObject(parent, *object, type)) {
                if(offset++ >= index) {
                    tail->next_metadata = &(*object)->metadata;
                    tail = tail->next_metadata;
                    numObjects++;
                }

                if(max_number > 0 && numObjects >= max_number) {
                    break;
                }
            }
        }

        if(numObjects > 0) {
            break;
        }
    }

    tail->next_metadata = NULL;

    if(metadata != NULL) {
        *metadata = temp.next_metadata;
    }

    return numObjects;
}

qint64 QListDB::getObjectSize(int ohfi)
{
    QMutexLocker locker(&mutex);

    CMAObject *obj = ohfiToObject(ohfi);
    return obj ? obj->metadata.size : -1;
}

int QListDB::getPathId(const char *name, int ohfi)
{
    QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {

        if(ohfi && (*root).first()->metadata.ohfi != ohfi) {
            continue;
        }
        CMAObject *obj = pathToObjectInternal(*root, name);

        if(obj) {
            return obj->metadata.ohfi;
        }
    }
    return 0;
}

int QListDB::insertObjectEntry(const QString &path, const QString &name, int parent_ohfi)
{
    QMutexLocker locker(&mutex);

    CMAObject *parent_obj = ohfiToObject(parent_ohfi);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        root_list *cat_list = &(*root);
        root_list::const_iterator it = qBinaryFind(cat_list->begin(), cat_list->end(), parent_obj, QListDB::lessThanComparator);

        if(it != cat_list->end()) {
            CMAObject *newobj = new CMAObject(parent_obj);

            // get the root object
            while(parent_obj->parent) {
                parent_obj = parent_obj->parent;
            }

            QFileInfo info(path, name);
            newobj->initObject(info, parent_obj->metadata.dataType);
            cat_list->append(newobj);
            return newobj->metadata.ohfi;
        }
    }

    return 0;
}

QString QListDB::getAbsolutePath(int ohfi)
{
    QMutexLocker locker(&mutex);
    CMAObject *obj = ohfiToObject(ohfi);
    return obj ? obj->path : NULL;
}

QString QListDB::getRelativePath(int ohfi)
{
    QMutexLocker locker(&mutex);
    CMAObject *obj = ohfiToObject(ohfi);
    return obj ? obj->metadata.path : NULL;
}

bool QListDB::renameObject(int ohfi, const QString &name)
{
    QMutexLocker locker(&mutex);

    CMAObject *root = ohfiToObject(ohfi);

    if(!root) {
        return false;
    }

    //rename the current object
    root->rename(name);
    QListDB::find_data iters;
    find(root->metadata.ohfi, iters);

    // rename the rest of the list only if has the renamed parent in some part of the chain
    while(iters.it != iters.end) {
        CMAObject *obj = *iters.it++;

        if(obj->hasParent(root)) {
            obj->refreshPath();
        }
    }

    return true;
}

void QListDB::setObjectSize(int ohfi, qint64 size)
{
    QMutexLocker locker(&mutex);
    CMAObject *obj = ohfiToObject(ohfi);

    if(obj) {
        obj->updateObjectSize(size);
    }
}

int QListDB::getRootId(int ohfi)
{
    QMutexLocker locker(&mutex);
    CMAObject *obj = ohfiToObject(ohfi);

    if(!obj) {
        return 0;
    }

    while(obj->parent) {
        obj = obj->parent;
    }

    return obj->metadata.ohfi;
}

int QListDB::getParentId(int ohfi)
{
    QMutexLocker locker(&mutex);
    CMAObject *obj = ohfiToObject(ohfi);

    return obj ? obj->metadata.ohfiParent : 0;
}
