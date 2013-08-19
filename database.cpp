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

#include "database.h"

#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QTextStream>
#include <QDebug>

#define OHFI_OFFSET 1000

const QStringList Database::image_types = QStringList() << "jpg" << "jpeg" << "png" << "tif" << "tiff" << "bmp" << "gif" << "mpo";
const QStringList Database::audio_types = QStringList() << "mp3" << "mp4" << "wav";
const QStringList Database::video_types = QStringList() << "mp4";

Database::Database(QObject *parent) :
    QObject(parent), mutex(QMutex::Recursive)
{
    CMARootObject::uuid = QSettings().value("lastAccountId", "ffffffffffffffff").toString();
}

Database::~Database()
{
    destroy();
}

void Database::setUUID(const QString uuid)
{
    CMARootObject::uuid = uuid;
    QSettings().setValue("lastAccountId", uuid);
}

int Database::create()
{
    int total_objects = 0;
    QMutexLocker locker(&mutex);
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
        total_objects += recursiveScanRootDirectory(list, obj, ohfi_array[i]);
        object_list[ohfi_array[i]] = list;
    }
    return total_objects;
}

CMAObject *Database::getParent(CMAObject *last_dir, const QString &current_path)
{
    while(last_dir && current_path != last_dir->path) {
        last_dir = last_dir->parent;
    }

    return last_dir;
}

int Database::scanRootDirectory(root_list &list, int ohfi_type)
{
    int total_objects = 0;
    CMAObject *last_dir = list.first();
    QDir dir(last_dir->path);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QDirIterator it(dir, QDirIterator::Subdirectories);

    while(it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

        if(info.isFile() && !checkFileType(info.absoluteFilePath(), ohfi_type)) {
            //qDebug("Excluding %s from database", info.absoluteFilePath().toStdString().c_str());
            continue;
        }

        CMAObject *obj = new CMAObject(getParent(last_dir, info.path()));
        obj->initObject(info);
        qDebug("Added %s to database with OHFI %d", obj->metadata.name, obj->metadata.ohfi);
        list << obj;

        if(obj->metadata.dataType & Folder) {
            last_dir = obj;
        } else {
            total_objects++;
        }
    }
    return total_objects;
}

int Database::recursiveScanRootDirectory(root_list &list, CMAObject *parent, int ohfi_type)
{
    int total_objects = 0;

    QDir dir(parent->path);
    dir.setSorting(QDir::Name | QDir::DirsFirst);
    QFileInfoList qsl = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &info, qsl) {
        if(info.isFile() && !checkFileType(info.absoluteFilePath(), ohfi_type)) {
            //qDebug("Excluding %s from database", info.absoluteFilePath().toStdString().c_str());>
        } else {
            CMAObject *obj = new CMAObject(parent);
            obj->initObject(info);
            qDebug("Added %s to database with OHFI %d", obj->metadata.name, obj->metadata.ohfi);
            list << obj;
            if(info.isDir()) {
                total_objects += recursiveScanRootDirectory(list, obj, ohfi_type);
            } else {
                total_objects++;
            }
        }
    }

    return total_objects;
}

void Database::destroy()
{
    QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        qDeleteAll(*root);
    }

    object_list.clear();
}

bool Database::removeInternal(root_list &list, const CMAObject *obj)
{
    bool found = false;
    QList<CMAObject *>::iterator it = list.begin();

    while(it != list.end()) {
        if(!found && (*it) == obj) {
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

bool Database::remove(const CMAObject *obj, int ohfi_root)
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

bool Database::lessThanComparator(const CMAObject *a, const CMAObject *b)
{
    return a->metadata.ohfi < b->metadata.ohfi;
}

bool Database::hasFilter(const CMARootObject *object,int ohfi)
{
    for(int i = 0; i < object->num_filters; i++) {
        if(object->filters[i].ohfi == ohfi) {
            return true;
        }
    }
    return false;
}

bool Database::findInternal(const root_list &list, int ohfi, find_data &data)
{
    if(hasFilter(static_cast<CMARootObject *>(list.first()), ohfi)) {
        data.it = list.begin();
    } else {
        CMAObject obj;
        obj.setOhfi(ohfi);
        data.it = qBinaryFind(list.begin(), list.end(), &obj, Database::lessThanComparator);
    }
    data.end = list.end();
    return data.it != data.end;
}

bool Database::find(int ohfi, Database::find_data &data)
{
    QMutexLocker locker(&mutex);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        if(findInternal(*root, ohfi, data)) {
            return true;
        }
    }
    return false;
}

void Database::append(int parent_ohfi, CMAObject *object)
{
    QMutexLocker locker(&mutex);
    CMAObject parent;
    parent.setOhfi(parent_ohfi);

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        root_list *cat_list = &(*root);
        root_list::const_iterator it = qBinaryFind(cat_list->begin(), cat_list->end(), &parent, Database::lessThanComparator);

        if(it != cat_list->end()) {
            cat_list->append(object);
            break;
        }
    }
}

CMAObject *Database::ohfiToObject(int ohfi)
{
    QMutexLocker locker(&mutex);
    find_data data;
    return find(ohfi, data) ? *data.it : NULL;
}

CMAObject *Database::pathToObjectInternal(const root_list &list, const char *path)
{
    // skip the first element since is the root element
    root_list::const_iterator skipped_first = ++list.begin();

    for(root_list::const_iterator obj = skipped_first; obj != list.end(); ++obj) {
        if(strcmp(path, (*obj)->metadata.path) == 0) {
            return (*obj);
        }
    }
    return NULL;
}

CMAObject *Database::pathToObject(const char *path, int ohfiRoot)
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

int Database::acceptFilteredObject(const CMAObject *parent, const CMAObject *current, int type)
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

    if(type & (VITA_DIR_TYPE_MASK_ALL | VITA_DIR_TYPE_MASK_SONGS)) {
        result = result && (current->metadata.dataType & File);
    } else if(type & (VITA_DIR_TYPE_MASK_REGULAR)) {
        result = (parent->metadata.ohfi == current->metadata.ohfiParent);
    }

    // TODO: Support other filter types
    return result;
}

void Database::dumpMetadataList(const metadata_t *p_head)
{
    while(p_head) {
        qDebug("Metadata: %s with OHFI %d", p_head->name, p_head->ohfi);
        p_head = p_head->next_metadata;
    }
}

int Database::filterObjects(int ohfiParent, metadata_t **p_head)
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

    int numObjects = 0;
    metadata_t temp = metadata_t();
    metadata_t *tail = &temp;

    for(map_list::iterator root = object_list.begin(); root != object_list.end(); ++root) {
        for(root_list::iterator object = (*root).begin(); object != (*root).end(); ++object) {
            if(acceptFilteredObject(parent, *object, type)) {
                tail->next_metadata = &(*object)->metadata;
                tail = tail->next_metadata;
                numObjects++;
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

    if(p_head) {
        dumpMetadataList(*p_head);
    }
    return numObjects;
}

bool Database::checkFileType(const QString path, int ohfi_root)
{
    switch(ohfi_root) {
    case VITA_OHFI_MUSIC:
        foreach(const QString &ext, audio_types) {
            if(path.endsWith(ext, Qt::CaseInsensitive)) {
                return true;
            }
        }
        break;
    case VITA_OHFI_PHOTO:
        foreach(const QString &ext, image_types) {
            if(path.endsWith(ext, Qt::CaseInsensitive)) {
                return true;
            }
        }
        break;
    case VITA_OHFI_VIDEO:
        foreach(const QString &ext, video_types) {
            if(path.endsWith(ext, Qt::CaseInsensitive)) {
                return true;
            }
        }
        break;
    default:
        return true;
    }
    return false;
}
