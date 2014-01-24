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
#include "qlistdb.h"
#include "cmaobject.h"

#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QTextStream>
#include <QThread>
#include <QDebug>

QListDB::QListDB() :
    mutex(QMutex::Recursive)
{
    QString uuid = QSettings().value("lastAccountId", "ffffffffffffffff").toString();
    CMARootObject::uuid  = uuid;
    thread = new QThread();
    timer = new QTimer();
    moveToThread(thread);
    thread->start();

    timer->setInterval(0);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(process()));
}

QListDB::~QListDB()
{
    destroy();
    timer->stop();
    delete timer;
    thread->quit();
    thread->wait();
    delete thread;
}

void QListDB::setUUID(const QString uuid)
{
    CMARootObject::uuid = uuid;
    QSettings().setValue("lastAccountId", uuid);
}

bool QListDB::reload(bool &prepared)
{
    if(mutex.tryLock()) {
        if(CMARootObject::uuid != "ffffffffffffffff") {
            timer->start();
            prepared = true;
        } else {
            mutex.unlock();
            prepared = false;
            return false;
        }
        return true;
    } else {
        return false;
    }
}

void QListDB::process()
{
    destroy();
    cancel_operation = false;
    int count = create();
    cancel_operation = false;
    qDebug("Added %i entries to the database", count);
    if(count < 0) {
        destroy();
    }
    emit updated(count);
    mutex.unlock();
}

void QListDB::cancelOperation()
{
    QMutexLocker locker(&cancel);
    cancel_operation = true;
}

bool QListDB::continueOperation()
{
    QMutexLocker locker(&cancel);
    return !cancel_operation;
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

        switch(ohfi_array[i]) {
        case VITA_OHFI_MUSIC:
            obj->initObject(settings.value("musicPath").toString());
            break;

        case VITA_OHFI_PHOTO:
            obj->initObject(settings.value("photoPath").toString());
            break;

        case VITA_OHFI_VIDEO:
            obj->initObject(settings.value("videoPath").toString());
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
        int dir_count = recursiveScanRootDirectory(list, obj, ohfi_array[i]);

        if(dir_count < 0) {
            return -1;
        }

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

void QListDB::destroy()
{
    //QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        CMARootObject *first = static_cast<CMARootObject *>((*root).takeFirst());
        delete first;
        qDeleteAll(*root);
    }

    object_list.clear();
}

bool QListDB::removeInternal(root_list &list, const CMAObject *obj)
{
    bool found = false;
    QList<CMAObject *>::iterator it = list.begin();

    while(it != list.end()) {
        if(!found && (*it) == obj) {
            // update the size of the parent objects
            (*it)->updateObjectSize(-(*it)->metadata.size);
            it = list.erase(it);
            found = true;
        } else if(found && (*it)->metadata.ohfiParent == obj->metadata.ohfi) {
            it = list.erase(it);
        } else {
            ++it;
        }
    }

    return found;
}

bool QListDB::remove(const CMAObject *obj, int ohfi_root)
{
    QMutexLocker locker(&mutex);

    if(ohfi_root) {
        return removeInternal(object_list[ohfi_root], obj);
    } else {
        for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
            if(removeInternal(*root, obj)) {
                return true;
            }
        }
    }
    return false;
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
    QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        if(findInternal(*root, ohfi, data)) {
            return true;
        }
    }
    return false;
}

void QListDB::append(int parent_ohfi, CMAObject *object)
{
    QMutexLocker locker(&mutex);
    CMAObject parent;
    parent.setOhfi(parent_ohfi);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        root_list *cat_list = &(*root);
        root_list::const_iterator it = qBinaryFind(cat_list->begin(), cat_list->end(), &parent, QListDB::lessThanComparator);

        if(it != cat_list->end()) {
            cat_list->append(object);
            break;
        }
    }
}

CMAObject *QListDB::ohfiToObject(int ohfi)
{
    QMutexLocker locker(&mutex);
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

CMAObject *QListDB::pathToObject(const char *path, int ohfiRoot)
{
    QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {

        if(ohfiRoot && (*root).first()->metadata.ohfi != ohfiRoot) {
            continue;
        }
        CMAObject *obj = pathToObjectInternal(*root, path);

        if(obj) {
            return obj;
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
    while(p_head) {
        qDebug("Metadata: %s with OHFI %d", p_head->name, p_head->ohfi);
        p_head = p_head->next_metadata;
    }
}

int QListDB::filterObjects(int ohfiParent, metadata_t **p_head, int index, int max_number)
{
    QMutexLocker locker(&mutex);
    CMARootObject *parent = static_cast<CMARootObject *>(ohfiToObject(ohfiParent));

    if(parent == NULL) {
        return 0;
    }

    int type = parent->metadata.type;

    if(parent->metadata.ohfi < OHFI_OFFSET && parent->filters) { // if we have filters
        if(ohfiParent == parent->metadata.ohfi) { // if we are looking at root
            return parent->getFilters(p_head);
        } else { // we are looking at a filter
            for(int j = 0; j < parent->num_filters; j++) {
                if(parent->filters[j].ohfi == ohfiParent) {
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

    if(p_head != NULL) {
        *p_head = temp.next_metadata;
    }

    return numObjects;
}
