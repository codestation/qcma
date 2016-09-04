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

#include "cmautils.h"
#include "sqlitedb.h"
#include "sforeader.h"

#ifdef FFMPEG_ENABLED
#include "avdecoder.h"
#endif

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#define QStandardPaths QDesktopServices
#define writableLocation storageLocation
#endif

#include <QSettings>
#include <QSqlQuery>

static const char create_adjacent[] = "CREATE TABLE IF NOT EXISTS adjacent_objects ("
                                      "parent_id INTEGER NOT NULL REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                      "child_id INTEGER NOT NULL REFERENCES object_node(object_id) ON DELETE CASCADE)";

static const char create_obj_node[] = "CREATE TABLE IF NOT EXISTS object_node ("
                                      "object_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "type INTEGER NOT NULL,"
                                      "data_type INTEGER NOT NULL,"
                                      "title TEXT,"
                                      "child_count INTEGER NOT NULL DEFAULT 0,"
                                      "reference_count INTEGER NOT NULL DEFAULT 0);";

static const char create_sources[] = "CREATE TABLE IF NOT EXISTS sources ("
                                     "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                     "path TEXT NOT NULL CHECK (LENGTH(path) > 0),"
                                     "size INTEGER,"
                                     "date_created TIMESTAMP,"
                                     "date_modified TIMESTAMP)";

static const char create_music[] = "CREATE TABLE IF NOT EXISTS music ("
                                   "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                   "file_format INTEGER,"
                                   "audio_bitrate INTEGER,"
                                   "audio_codec INTEGER,"
                                   "duration INTEGER,"
                                   "genre_id INTEGER REFERENCES object_node(object_id) ON DELETE SET NULL,"
                                   "track_id INTEGER REFERENCES object_node(object_id) ON DELETE SET NULL,"
                                   "artist_id INTEGER REFERENCES object_node(object_id) ON DELETE SET NULL,"
                                   "album_id INTEGER REFERENCES object_node(object_id) ON DELETE SET NULL,"
                                   "artist TEXT,"
                                   "album TEXT,"
                                   "track_number INTEGER)";

static const char create_apps[] = "CREATE TABLE IF NOT EXISTS application ("
                                  "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                  "title TEXT NOT NULL CHECK (LENGTH(title) > 0),"
                                  "app_type INTEGER)";

static const char create_virtual[] = "CREATE TABLE IF NOT EXISTS virtual_nodes ("
                                     "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                     "app_type INTEGER)";

static const char create_photos[] = "CREATE TABLE IF NOT EXISTS photos ("
                                    "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                    "date_created TIMESTAMP,"
                                    "month_created TEXT,"
                                    "file_format INTEGER,"
                                    "photo_codec INTEGER,"
                                    "width INTEGER,"
                                    "height INTEGER)";

static const char create_videos[] = "CREATE TABLE IF NOT EXISTS videos ("
                                    "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                    "file_format INTEGER,"
                                    "parental_level INTEGER,"
                                    "explanation TEXT,"
                                    "copyright TEXT,"
                                    "width INTEGER,"
                                    "height INTEGER,"
                                    "video_codec INTEGER,"
                                    "video_bitrate INTEGER,"
                                    "audio_codec INTEGER,"
                                    "audio_bitrate INTEGER,"
                                    "duration INTEGER)";

static const char create_savedata[] = "CREATE TABLE IF NOT EXISTS savedata ("
                                      "object_id INTEGER PRIMARY KEY REFERENCES object_node(object_id) ON DELETE CASCADE,"
                                      "detail TEXT,"
                                      "dir_name TEXT,"
                                      "title TEXT,"
                                      "date_updated TIMESTAMP)";

static const char create_trigger_node[] = "CREATE TRIGGER IF NOT EXISTS trg_objnode_deletechilds BEFORE DELETE ON object_node "
        "FOR EACH ROW BEGIN "
        "DELETE FROM object_node WHERE object_id IN "
        "(SELECT child_id FROM adjacent_objects WHERE parent_id == OLD.object_id);"
        "END";

static const char create_trigger_adjins[] = "CREATE TRIGGER IF NOT EXISTS trg_adjacentobjects_ins AFTER INSERT ON adjacent_objects "
        "FOR EACH ROW BEGIN "
        "UPDATE object_node SET child_count = child_count + 1 WHERE object_id = NEW.parent_id;"
        "UPDATE object_node SET reference_count = reference_count + 1 WHERE object_id = NEW.child_id;"
        "END";

static const char create_trigger_adjdel[] = "CREATE TRIGGER IF NOT EXISTS trg_adjacentobjects_del AFTER DELETE ON adjacent_objects "
        "FOR EACH ROW BEGIN "
        "UPDATE object_node SET child_count = child_count - 1 WHERE object_id = OLD.parent_id;"
        "UPDATE object_node SET reference_count = reference_count - 1 WHERE object_id = OLD.child_id;"
        "DELETE FROM object_node WHERE object_id = OLD.parent_id AND child_count <= 0;"
        "DELETE FROM object_node WHERE object_id = OLD.child_id AND reference_count <= 0;"
        "END";

static const char *table_list[] = {
    create_adjacent, create_obj_node, create_sources,
    create_music, create_photos, create_videos, create_savedata, create_apps, create_virtual
};

static const char *trigger_list[] = {
    create_trigger_node, create_trigger_adjins, create_trigger_adjdel
};

static const int ohfi_array[] = { VITA_OHFI_MUSIC, VITA_OHFI_PHOTO, VITA_OHFI_VIDEO,
                                  VITA_OHFI_BACKUP, VITA_OHFI_VITAAPP, VITA_OHFI_PSPAPP,
                                  VITA_OHFI_PSPSAVE, VITA_OHFI_PSXAPP, VITA_OHFI_PSMAPP
                                };

SQLiteDB::SQLiteDB(QObject *obj_parent) :
    Database(obj_parent)
{
    m_uuid = QSettings().value("lastAccountId", "ffffffffffffffff").toString();
    thread = new QThread();
    moveToThread(thread);
    timer = new QTimer();
    thread->start();
    timer->setInterval(0);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(process()));

    // fetch a configured database path if it exists
    QString db_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    db_path = QSettings().value("databasePath", db_path).toString();
    QDir(QDir::root()).mkpath(db_path);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_path + QDir::separator() + "qcma.sqlite");
}

SQLiteDB::~SQLiteDB()
{
    db.close();
    timer->stop();
    delete timer;
    thread->quit();
    thread->wait();
    delete thread;
}


void SQLiteDB::setUUID(const QString &uuid)
{
    m_uuid = uuid;
    QSettings().setValue("lastAccountId", uuid);
}

bool SQLiteDB::load()
{
    bool success = false;

    if(db.open()) {
        QSqlQuery query;
        query.exec("PRAGMA foreign_keys = ON");
        query.exec("PRAGMA synchronous = OFF");
        query.exec("PRAGMA user_version = 1");
        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA recursive_triggers = true");
        query.exec("PRAGMA vacuum_db.synchronous = OFF");

        qDebug() << "Database created/registered";
        success = !initialize();
    } else {
        const QSqlError error = db.lastError();
        qWarning() << "Error opening connection to the database:" << error.text();
    }

    return success;
}

bool SQLiteDB::rescan()
{
    if(mutex.tryLock(1000)) {
        if(m_uuid != "ffffffffffffffff") {
            timer->start();
            return true;
        } else {
            mutex.unlock();
            return false;
        }
    }
    return false;
}

void SQLiteDB::clear()
{
    db.close();
    //QSqlDatabase::removeDatabase("QSQLITE");
    QString db_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    db_path = QSettings().value("databasePath", db_path).toString();
    QFile(db_path + QDir::separator() + "qcma.sqlite").remove();
    load();
}

bool SQLiteDB::initialize()
{
    QSqlQuery query;

    for(unsigned int i = 0; i < sizeof(table_list) / sizeof(const char *); i++) {
        if(!query.exec(table_list[i])) {
            qDebug() << query.lastError();
            return false;
        }
    }

    for(unsigned int i = 0; i < sizeof(trigger_list) / sizeof(const char *); i++) {
        if(!query.exec(trigger_list[i])) {
            qDebug() << query.lastError();
            return false;
        }
    }

    // force object_id to start at 256
    if(query.exec("INSERT INTO object_node (object_id, data_type, type) VALUES (255, 0, 0)")) {
        query.exec("DELETE FROM object_node WHERE object_id = 255");
    }
    return true;
}

QSqlError SQLiteDB::getLastError()
{
    return db.lastError();
}

int SQLiteDB::create()
{
    int total_objects = 0;

    db.transaction();

    if(!insertVirtualEntries()) {
        db.rollback();
        return -1;
    }

    for(int i = 0, max = sizeof(ohfi_array) / sizeof(int); i < max; i++) {
        QString base_path = getBasePath(ohfi_array[i]);
        int dir_count = recursiveScanRootDirectory(base_path, NULL, ohfi_array[i], ohfi_array[i]);

        if(dir_count < 0) {
            db.rollback();
            return -1;
        }

        //qDebug("Added %i objects for OHFI %#02X", dir_count, ohfi_array[i]);

        total_objects += dir_count;
    }
    db.commit();
    return total_objects;
}

QString SQLiteDB::getBasePath(int root_ohfi)
{
    QString base_path;
    QSettings settings;

    switch(root_ohfi) {
    case VITA_OHFI_MUSIC:
        base_path = settings.value("musicPath").toString();
        break;
    case VITA_OHFI_VIDEO:
        base_path = settings.value("videoPath").toString();
        break;
    case VITA_OHFI_PHOTO:
        base_path = settings.value("photoPath").toString();
        break;
    case VITA_OHFI_BACKUP:
        base_path = settings.value("appsPath").toString() + "/SYSTEM/" + m_uuid;
        break;
    case VITA_OHFI_VITAAPP:
        base_path = settings.value("appsPath").toString() + "/APP/" + m_uuid;
        break;
    case VITA_OHFI_PSPAPP:
        base_path = settings.value("appsPath").toString() + "/PGAME/" + m_uuid;
        break;
    case VITA_OHFI_PSPSAVE:
        base_path = settings.value("appsPath").toString() + "/PSAVEDATA/" + m_uuid;
        break;
    case VITA_OHFI_PSXAPP:
        base_path = settings.value("appsPath").toString() + "/PSGAME/" + m_uuid;
        break;
    case VITA_OHFI_PSMAPP:
        base_path = settings.value("appsPath").toString() + "/PSM/" + m_uuid;
        break;
    }
    return base_path;
}

int SQLiteDB::recursiveScanRootDirectory(const QString &base_path, const QString &rel_path, int parent_ohfi, int root_ohfi)
{
    int total_objects = 0;

    QDir dir(base_path + "/" + rel_path);
    dir.setSorting(QDir::Name);
    QFileInfoList qsl = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &info, qsl) {

        if(!continueOperation()) {
            return -1;
        }

        //qDebug() << "Processing " << info.fileName();

        QString rel_name = rel_path.isNull() ? info.fileName() : rel_path + "/" + info.fileName();

        int ohfi = insertObjectEntryInternal(base_path, rel_name, parent_ohfi, root_ohfi);

        if(ohfi > 0) {
            // update progress dialog
            if(info.isDir()) {
                emit directoryAdded(base_path + "/" + rel_name);
                int inserted = recursiveScanRootDirectory(base_path, rel_name, ohfi, root_ohfi);
                if(inserted < 0) {
                    return -1;
                }

                total_objects += inserted;
                qint64 dirsize = getChildenTotalSize(ohfi);
                setObjectSize(ohfi, dirsize);
            } else if(info.isFile()) {
                emit fileAdded(info.fileName());
                total_objects++;
            }
        }
    }

    return total_objects;
}

int SQLiteDB::insertObjectEntry(const QString &path, const QString &name, int parent_ohfi)
{
    int ohfi;
    int type;

    if((type = getObjectType(parent_ohfi)) == 0) {
        return 0;
    }
    db.transaction();
    if((ohfi = insertObjectEntryInternal(path, name, parent_ohfi, type)) == 0) {
        db.rollback();
    }
    db.commit();

    return ohfi;
}

int SQLiteDB::insertObjectEntryInternal(const QString &path, const QString &name, int parent_ohfi, int root_ohfi)
{
    int ohfi = 0;
    QFileInfo info(path, name);

    if(info.isDir()) {
        ohfi = insertDefaultEntry(path, name, info.fileName(), parent_ohfi, Folder);
        switch(parent_ohfi) {
        case VITA_OHFI_VITAAPP:
        case VITA_OHFI_PSPAPP:
        case VITA_OHFI_PSXAPP:
        case VITA_OHFI_PSMAPP:
        case VITA_OHFI_BACKUP:
            insertApplicationEntry(name, ohfi, parent_ohfi);
        }
    } else {
        switch(root_ohfi) {
        case VITA_OHFI_MUSIC:
            ohfi = insertMusicEntry(path, name, parent_ohfi, File | Music);
            break;
        case VITA_OHFI_PHOTO:
            ohfi = insertPhotoEntry(path, name, parent_ohfi, File | Photo);
            break;
        case VITA_OHFI_VIDEO:
            ohfi = insertVideoEntry(path, name, parent_ohfi, File | Video);
            break;
        case VITA_OHFI_PSPSAVE:
            ohfi = insertSavedataEntry(path, name, parent_ohfi, File | SaveData);
            break;
        case VITA_OHFI_VITAAPP:
        case VITA_OHFI_PSPAPP:
        case VITA_OHFI_PSXAPP:
        case VITA_OHFI_PSMAPP:
        case VITA_OHFI_BACKUP:
            ohfi = insertDefaultEntry(path, name, info.fileName(), parent_ohfi, File | App);
        }
    }
    return ohfi;
}


int SQLiteDB::getPathId(const QString &path)
{
    QSqlQuery query(QString("SELECT object_id from sources WHERE path = %1").arg(path));
    if(query.next()) {
        return query.value(0).toInt();
    } else {
        qDebug() << query.lastError();
        return -1;
    }
}

QString SQLiteDB::getPathFromId(int ohfi)
{
    QSqlQuery query(QString("SELECT path FROM sources WHERE object_id = %1").arg(ohfi));
    if(query.next()) {
        return query.value(0).toString();
    } else {
        qDebug() << query.lastError();
        return QString();
    }
}

bool SQLiteDB::updateSize(int ohfi, quint64 size)
{
    QSqlQuery query(QString("UPDATE sources SET size = %1 WHERE object_id == %2").arg(size).arg(ohfi));
    return query.exec();
}

bool SQLiteDB::deleteEntry(int ohfi)
{
    QSqlQuery query(QString("DELETE FROM object_node WHERE object_id == %1").arg(ohfi));
    bool ret = query.exec();
    if(!ret) {
        qDebug() << query.lastError();
    }
    return ret;
}

bool SQLiteDB::updateAdjacencyList(int ohfi, int id_parent)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM adjacent_objects WHERE parent_id == :parent_id AND child_id == :child_id");
    query.bindValue(0, id_parent);
    query.bindValue(1, ohfi);

    if(query.exec() && query.next()) {
        return true;
    }

    query.prepare("INSERT INTO adjacent_objects (parent_id, child_id)"
                  "VALUES (:parentid, :child_id)");
    query.bindValue(0, id_parent);
    query.bindValue(1, ohfi);
    bool ret = query.exec();
    if(!ret) {
        qDebug() << query.lastError();
    }
    return ret;
}

int SQLiteDB::insertDefaultEntry(const QString &path, const QString &name, const QString &title, int id_parent, int type)
{
    int ohfi = 0;

    if((ohfi = insertNodeEntry(title, VITA_DIR_TYPE_MASK_REGULAR, type)) == 0) {
        return 0;
    }

    if(id_parent >= OHFI_BASE_VALUE && !updateAdjacencyList(ohfi, id_parent)) {
        return 0;
    }

    if(!name.isNull() && !insertSourceEntry(ohfi, path, name)) {
        return 0;
    }
    return ohfi;
}

int SQLiteDB::insertNodeEntry(const QString &title, int type, int data_type)
{
    QSqlQuery query;

    query.prepare("INSERT INTO object_node (type, data_type, title) VALUES (:type, :data_type, :title)");
    query.bindValue(0, type);
    query.bindValue(1, data_type);
    query.bindValue(2, title);

    if(!query.exec() || !query.exec("SELECT last_insert_rowid()") || !query.next()) {
        qDebug() << query.lastError();
        return 0;
    }

    return query.value(0).toInt();
}

bool SQLiteDB::insertSourceEntry(uint object_id, const QString &path, const QString &name)
{
    QVariant size, date_created, date_modified;

    QFileInfo info(path, name);
    if(info.isFile()) {
        size = QVariant(info.size());
        date_created = QVariant(info.created().toUTC().toTime_t());
    } else {
        size = QVariant(QVariant::LongLong);
        date_created = QVariant(QVariant::UInt);
    }

    date_modified = QVariant(info.lastModified().toUTC().toTime_t());

    QSqlQuery query;
    query.prepare("REPLACE INTO sources (object_id, path, size, date_created, date_modified)"
                  "VALUES (:object_id, :path, :size, :date_created, :date_modified)");
    query.bindValue(0, object_id);
    query.bindValue(1, name);
    query.bindValue(2, size);
    query.bindValue(3, date_created);
    query.bindValue(4, date_modified);
    bool ret = query.exec();
    if(!ret) {
        qDebug() << query.lastError();
    }
    return ret;
}

uint SQLiteDB::insertMusicEntry(const QString &path, const QString &name, int id_parent, int type)
{
#ifndef FFMPEG_ENABLED
    Q_UNUSED(path);
    Q_UNUSED(name);
    Q_UNUSED(id_parent);
    Q_UNUSED(type);

    return 0;
#else
    bool ok;
    int ohfi;
    AVDecoder decoder;
    quint64 duration;
    const char *artist, *album, *albumartist, *genre, *track, *title;
    int file_format, audio_codec, audio_bitrate, genre_id, artist_id, track_id, album_id, track_number;

    int cma_file_type = checkFileType(name, VITA_OHFI_MUSIC);
    if(cma_file_type < 0) {
        //qDebug() << "Excluding from database:" << path;
        return 0;
    }

    if(!decoder.open(path + "/" + name)) {
        return 0;
    }

    album = decoder.getMetadataEntry("album");
    genre = decoder.getMetadataEntry("genre");
    artist = decoder.getMetadataEntry("artist");
    albumartist = decoder.getMetadataEntry("album_artist");
    track = decoder.getMetadataEntry("track");

    track_number = QString(track).split("/")[0].toInt(&ok);
    if(!ok) {
        // set track number to 1 by default
        track_number = 1;
    }

    if(decoder.loadCodec(AVDecoder::CODEC_AUDIO)) {
        audio_bitrate = decoder.getBitrate();
    } else {
        audio_bitrate = 0;
    }

    duration = decoder.getDuration();

    file_format = audio_list[cma_file_type].file_format;
    audio_codec = audio_list[cma_file_type].file_codec;

    QByteArray basename = QFileInfo(name).baseName().toUtf8();
    title = decoder.getMetadataEntry("title", basename.constData());

    if((ohfi = insertDefaultEntry(path, name, title, id_parent, type)) == 0) {
        return 0;
    }

    if(genre) {
        genre_id = insertNodeEntry(genre, VITA_DIR_TYPE_MASK_GENRES, type);
        if(!updateAdjacencyList(ohfi, genre_id)) {
            return 0;
        }
    } else {
        genre_id = 0;
    }

    if(artist) {
        track_id = insertNodeEntry(artist, VITA_DIR_TYPE_MASK_ARTISTS, type);
        if(!updateAdjacencyList(ohfi, track_id)) {
            return 0;
        }
    } else {
        track_id = 0;
    }

    if(albumartist) {
        artist_id = insertNodeEntry(albumartist, VITA_DIR_TYPE_MASK_REGULAR, type);
        if(!updateAdjacencyList(ohfi, artist_id)) {
            return 0;
        }
    } else {
        artist_id = 0;
    }

    if(album) {
        album_id = insertNodeEntry(album, VITA_DIR_TYPE_MASK_ALBUMS, type);

        if(track_id && !updateAdjacencyList(ohfi, album_id)) {
            return 0;
        }
        if(track_id && !updateAdjacencyList(album_id, track_id)) {
            return 0;
        }
        if(artist_id && !updateAdjacencyList(album_id, artist_id)) {
            return 0;
        }
    } else {
        album_id = 0;
    }

    QSqlQuery query;
    query.prepare("REPLACE INTO music"
                  "(object_id, file_format, audio_codec, audio_bitrate, duration, genre_id, artist_id, album_id, track_id, artist, album, track_number)"
                  "VALUES (:object_id, :file_format, :audio_codec, :audio_bitrate, :duration, :genre_id, :artist_id, :album_id, :track_id, :artist, :album, :track_number)");

    query.bindValue(0, ohfi);
    query.bindValue(1, file_format);
    query.bindValue(2, audio_codec);
    query.bindValue(3, audio_bitrate);
    query.bindValue(4, duration);
    query.bindValue(5, genre_id ? genre_id : QVariant(QVariant::Int));
    query.bindValue(6, artist_id ? artist_id : QVariant(QVariant::Int));
    query.bindValue(7, album_id ? album_id : QVariant(QVariant::Int));
    query.bindValue(8, track_id ? track_id : QVariant(QVariant::Int));
    query.bindValue(9, artist);
    query.bindValue(10, album);
    query.bindValue(11, track_number);

    if(!query.exec()) {
        return 0;
    }

    return ohfi;
#endif
}

uint SQLiteDB::insertVideoEntry(const QString &path, const QString &name, int id_parent, int type)
{
#ifndef FFMPEG_ENABLED
    Q_UNUSED(path);
    Q_UNUSED(name);
    Q_UNUSED(id_parent);
    Q_UNUSED(type);

    return 0;
#else
    int ohfi;
    AVDecoder decoder;
    quint64 duration;
    int file_format, parental_level, width, height, video_codec, video_bitrate, audio_codec, audio_bitrate;
    const char *explanation, *copyright, *title;

    if(!decoder.open(path + "/" + name) || !decoder.loadCodec(AVDecoder::CODEC_VIDEO)) {
        return 0;
    }

    int cma_file_type = checkFileType(name, VITA_OHFI_VIDEO);
    if(cma_file_type < 0) {
        //qDebug() << "Excluding from database:" << path;
        return 0;
    }

    parental_level = 0;
    explanation = decoder.getMetadataEntry("comments", "");
    copyright = decoder.getMetadataEntry("copyright", "");
    width = decoder.getWidth();
    height = decoder.getHeight();
    duration = decoder.getDuration();
    video_codec = CODEC_TYPE_AVC;
    video_bitrate = decoder.getBitrate();
    file_format = video_list[cma_file_type].file_format;

    if(decoder.loadCodec(AVDecoder::CODEC_AUDIO)) {
        audio_codec = CODEC_TYPE_AAC;
        audio_bitrate = decoder.getBitrate();
    } else {
        audio_codec = 0;
        audio_bitrate = 0;
    }

    QByteArray basename = QFileInfo(name).baseName().toUtf8();
    //title = decoder.getMetadataEntry("title", basename.constData());
    title = basename.constData();


    if((ohfi = insertDefaultEntry(path, name, title, id_parent, type)) == 0) {
        return 0;
    }

    QSqlQuery query;
    query.prepare("REPLACE INTO videos"
                  "(object_id, file_format, parental_level, explanation, copyright, width, height, video_codec, video_bitrate, audio_codec, audio_bitrate, duration)"
                  "VALUES (:object_id, :file_format, :parental_level, :explanation, :copyright, :width, :height, :video_codec, :video_bitrate, :audio_codec, :audio_bitrate, :duration)");

    query.bindValue(0, ohfi);
    query.bindValue(1, file_format);
    query.bindValue(2, parental_level);
    query.bindValue(3, explanation);
    query.bindValue(4, copyright);
    query.bindValue(5, width);
    query.bindValue(6, height);
    query.bindValue(7, video_codec);
    query.bindValue(8, video_bitrate);
    query.bindValue(9, audio_codec);
    query.bindValue(10, audio_bitrate);
    query.bindValue(11, duration);

    if(!query.exec()) {
        qDebug() << query.lastError().text();
        return 0;
    }

    return ohfi;
#endif
}

uint SQLiteDB::insertPhotoEntry(const QString &path, const QString &name, int id_parent, int type)
{
    int ohfi;
    //QImage img;
    uint date_created;
    int width, height, file_format, photo_codec;

    int cma_file_type = checkFileType(name, VITA_OHFI_PHOTO);
    if(cma_file_type < 0) {
        //qDebug() << "Excluding from database:" << path;
        return 0;
    }

    //if(!img.load(path + "/" + name)) {
    //    return 0;
    //}

    QDateTime date = QFileInfo(path + "/" + name).created();
    date_created = date.toUTC().toTime_t();
    QString month_created = date.toString("yyyy/MM");

    width = 0; //img.width();
    height = 0; //img.height();
    file_format = 0; //photo_list[cma_file_type].file_format;
    photo_codec = 0; //photo_list[cma_file_type].file_codec;

    QByteArray basename = QFileInfo(name).baseName().toUtf8();

    if((ohfi = insertDefaultEntry(path, name, basename, id_parent, type)) == 0) {
        return 0;
    }

    QSqlQuery query;
    query.prepare("REPLACE INTO photos"
                  "(object_id, date_created, file_format, photo_codec, width, height, month_created)"
                  "VALUES (:object_id, :date_created, :file_format, :photo_codec, :width, :height, :month_created)");

    query.bindValue(0, ohfi);
    query.bindValue(1, date_created);
    query.bindValue(2, file_format);
    query.bindValue(3, photo_codec);
    query.bindValue(4, width);
    query.bindValue(5, height);
    query.bindValue(6, month_created);

    if(!query.exec()) {
        return 0;
    }

    return ohfi;
}

uint SQLiteDB::insertSavedataEntry(const QString &path, const QString &name, int id_parent, int type)
{
    int ohfi;
    SfoReader reader;
    uint date_updated = 0;
    const char *title = NULL;
    const char *savedata_detail = NULL;
    const char *savedata_directory = NULL;

    QString file_name = QFileInfo(name).fileName();
    QByteArray utf8name = file_name.toUtf8();

    if(file_name.endsWith(".sfo", Qt::CaseInsensitive) && reader.load(path + "/" + name)) {
        title = reader.value("TITLE", utf8name.constData());
        savedata_detail = reader.value("SAVEDATA_DETAIL", "");
        savedata_directory = reader.value("SAVEDATA_DIRECTORY", utf8name.constData());
        date_updated = QFileInfo(path + "/" + name).lastModified().toUTC().toTime_t();
    }

    if((ohfi = insertDefaultEntry(path, name, title, id_parent, type)) == 0) {
        return 0;
    }

    if(!title) {
        return ohfi;
    }

    QSqlQuery query;
    query.prepare("REPLACE INTO savedata"
                  "(object_id, detail, dir_name, title, date_updated)"
                  "VALUES (:object_id, :detail, :dir_name, :title, :updated)");

    query.bindValue(0, id_parent);
    query.bindValue(1, savedata_detail);
    query.bindValue(2, savedata_directory);
    query.bindValue(3, title);
    query.bindValue(4, date_updated);

    if(!query.exec()) {
        return 0;
    }

    return ohfi;
}


bool SQLiteDB::insertApplicationEntry(const QString &name, int ohfi, int app_type)
{
    QString title_id = QFileInfo(name).fileName();

    QSqlQuery query;
    query.prepare("REPLACE INTO application"
                  "(object_id, title, app_type)"
                  "VALUES (:object_id, :title, :app_type)");

    query.bindValue(0, ohfi);
    query.bindValue(1, title_id);
    query.bindValue(2, app_type);

    if(!query.exec()) {
        return false;
    }

    return true;
}

int SQLiteDB::childObjectCount(int parent_ohfi)
{
    QSqlQuery query;
    query.prepare("SELECT count(child_id) FROM adjacent_objects WHERE parent_id = :object_id");
    query.bindValue(0, parent_ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return -1;
    } else {
        return query.value(0).toInt();
    }
}

bool SQLiteDB::deleteEntry(int ohfi, int root_ohfi)
{
    Q_UNUSED(root_ohfi);
    qDebug("Deleting node: %i", ohfi);

    QSqlQuery query("DELETE FROM object_node WHERE object_id = :object_id");
    query.bindValue(0, ohfi);
    return query.exec();
}

void SQLiteDB::fillMetadata(const QSqlQuery &query, metadata_t &metadata)
{
    metadata.ohfi = query.value(0).toInt(); // ohfi
    metadata.ohfiParent = query.value(1).toInt(); // parent
    metadata.path = strdup(query.value(2).toByteArray().constData()); // path
    metadata.name = strdup(query.value(3).toByteArray().constData()); // name
    metadata.type = VITA_DIR_TYPE_MASK_REGULAR;
    metadata.dataType = (DataType)query.value(5).toInt(); // data_type
    metadata.size = query.value(6).toULongLong(); // size
    metadata.dateTimeCreated = query.value(7).toInt(); // date_created
    metadata.next_metadata = NULL;
    //TODO: fill the rest of the metadata
}

bool SQLiteDB::getObjectMetadata(int ohfi, metadata_t &metadata)
{
    QSqlQuery query(
        "SELECT "
        "t0.object_id as ohfi,"
        "t1.parent_id as parent,"
        "t2.path as path,"
        "t0.title as name,"
        "t0.type as type,"
        "t0.data_type as data_type,"
        "t2.size as size,"
        "t2.date_created as date_created "
        "FROM object_node t0 "
        "JOIN adjacent_objects t1 ON t1.child_id = t0.object_id "
        "JOIN sources t2 ON t2.object_id = t0.object_id "
        "WHERE t0.object_id = :object_id");
    query.bindValue(0, ohfi);

    if(query.exec() && query.next()) {
        fillMetadata(query, metadata);
        return true;
    } else {
        qDebug() << query.lastError();
        return false;
    }
}

int SQLiteDB::getObjectMetadatas(int parent_ohfi, metadata_t **metadata, int index, int max_number)
{
    if(metadata == NULL) {
        return childObjectCount(parent_ohfi);
    }

    if(parent_ohfi < OHFI_BASE_VALUE) {
        return getRootItems(parent_ohfi, metadata);
    }

    int count = 0;
    QString query_str(
        "SELECT "
        "t0.child_id as ohfi,"
        "t0.parent_id as parent,"
        "t2.path as path,"
        "t1.title as name,"
        "t1.type as type,"
        "t1.data_type as data_type,"
        "t2.size as size,"
        "t2.date_created as date_created "
        "FROM adjacent_objects t0 "
        "JOIN object_node t1 ON t0.child_id = t1.object_id "
        "JOIN sources t2 ON t0.child_id = t2.object_id "
        "WHERE t0.parent_id = :parent_id ");

    if(max_number > 0) {
        query_str += "LIMIT :limit OFFSET :offset";
    }

    QSqlQuery query(query_str);
    query.bindValue(0, parent_ohfi);

    if(max_number > 0) {
        query.bindValue(1, max_number);
        query.bindValue(2, index);
    }

    if(query.exec()) {
        metadata_t **last = &*metadata;
        while(query.next()) {
            metadata_t *meta = new metadata_t();
            fillMetadata(query, *meta);
            *last = meta;
            last = &meta->next_metadata;
            count++;
        }
    } else {
        qDebug() << query.lastError();
    }
    return count;
}

qint64 SQLiteDB::getObjectSize(int ohfi)
{
    if(ohfi < OHFI_BASE_VALUE) {
        return getChildenTotalSize(ohfi);
    }

    QSqlQuery query;
    query.prepare("SELECT size FROM sources WHERE object_id = :object_id");
    query.bindValue(0, ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return -1;
    } else {
        return query.value(0).toInt();
    }
}

int SQLiteDB::getPathId(const char *name, int ohfi)
{
    //FIXME: use ohfi to filter by category
    Q_UNUSED(ohfi);
    QSqlQuery query;

    query.prepare("SELECT object_id FROM sources WHERE path = :path");
    query.bindValue(0, name);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return 0;
    } else {
        return query.value(0).toInt();
    }
}

QString SQLiteDB::getAbsolutePath(int ohfi)
{
    int root_ohfi = ohfi < OHFI_BASE_VALUE ? ohfi : getRootId(ohfi);
    QString base_path = getBasePath(root_ohfi);
    QString rel_path = getRelativePath(ohfi);
    return rel_path.isNull() ? base_path : base_path + "/" + rel_path;
}

QString SQLiteDB::getRelativePath(int ohfi)
{
    QSqlQuery query;
    query.prepare("SELECT path FROM sources WHERE object_id = :object_id");
    query.bindValue(0, ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return NULL;
    } else {
        return query.value(0).toString();
    }
}

bool SQLiteDB::updateObjectPath(int ohfi, const QString &name)
{
    int parent_ohfi = getParentId(ohfi);
    QString parent_path, file;

    if(name.isNull()) {
        QSqlQuery name_query("SELECT title FROM object_node WHERE object_id = :object_id");
        name_query.bindValue(0, ohfi);
        if(!name_query.exec() || !name_query.next()) {
            return false;
        }
        file = name_query.value(0).toString();
    } else {
        file = name;
    }

    if(parent_ohfi >= OHFI_BASE_VALUE) {
        parent_path = getRelativePath(parent_ohfi);
        if(parent_path.isNull()) {
            parent_path = file;
        } else {
            parent_path += "/" + file;
        }
    } else {
        parent_path = file;
    }

    QSqlQuery query("UPDATE sources SET path = :path WHERE object_id = :object_id");
    query.bindValue(0, parent_path);
    query.bindValue(1, ohfi);

    if(!query.exec()) {
        return false;
    }

    DataType type = (DataType)getObjectType(ohfi);

    if(type & Folder) {
        QSqlQuery child_query("SELECT child_id, data_type FROM adjacent_objects "
                              "JOIN object_node ON object_id = child_id"
                              "WHERE parent_id = :parent_id");
        child_query.bindValue(0, ohfi);

        if(query.exec()) {
            while(query.next()) {
                int child_ohfi = query.value(0).toInt();
                if(!updateObjectPath(child_ohfi, NULL)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool SQLiteDB::renameObject(int ohfi, const QString &name)
{
    QSqlQuery query("UPDATE object_node SET title = :title WHERE object_id = :object_id");
    query.bindValue(0, name);
    query.bindValue(1, ohfi);

    if(!query.exec()) {
        return false;
    }

    return updateObjectPath(ohfi, name);
}

void SQLiteDB::setObjectSize(int ohfi, qint64 size)
{
    QSqlQuery query;
    query.prepare("UPDATE sources SET size = :size WHERE object_id = :object_id");
    query.bindValue(0, size);
    query.bindValue(1, ohfi);
    if(!query.exec()) {
        qDebug() << query.lastError();
    }
}

qint64 SQLiteDB::getChildenTotalSize(int ohfi)
{
    QSqlQuery query;
    query.prepare("SELECT SUM(t0.size) FROM sources t0 "
                  "JOIN adjacent_objects t1 ON t0.object_id = t1.child_id "
                  "where t1.parent_id = :parent_id");
    query.bindValue(0, ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return -1;
    } else {
        return query.value(0).toLongLong();
    }
}

int SQLiteDB::getRootId(int ohfi)
{
    QSqlQuery query;
    int root_ohfi = ohfi;

    query.prepare("SELECT parent_id FROM adjacent_objects WHERE child_id = :child_id");
    while(root_ohfi >= OHFI_BASE_VALUE) {
        query.bindValue(0, root_ohfi);
        if(!query.exec() || !query.next()) {
            qDebug() << query.lastError();
            root_ohfi = 0;
        } else {
            root_ohfi = query.value(0).toInt();
        }
    }
    return root_ohfi;
}

int SQLiteDB::getObjectType(int ohfi)
{
    QSqlQuery query;
    query.prepare("SELECT type FROM object_node WHERE object_id = :object_id");
    query.bindValue(0, ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return 0;
    } else {
        return query.value(0).toInt();
    }
}

int SQLiteDB::getParentId(int ohfi)
{
    QSqlQuery query;
    query.prepare("SELECT parent_id FROM adjacent_objects WHERE child_id = :child_id");
    query.bindValue(0, ohfi);
    if(!query.exec() || !query.next()) {
        qDebug() << query.lastError();
        return 0;
    } else {
        return query.value(0).toInt();
    }
}

void SQLiteDB::freeMetadata(metadata_t *metadata)
{
    while(metadata) {
        metadata_t *current = metadata;
        metadata = metadata->next_metadata;
        delete current;
    }
}

int SQLiteDB::getRootItems(int root_ohfi, metadata_t **metadata)
{
    QSqlQuery query;
    int count = 0;

    switch(root_ohfi) {
    case VITA_OHFI_MUSIC:
        //query = QSqlQuery("SELECT * FROM music");
        break;
    case VITA_OHFI_PHOTO:
        //query = QSqlQuery("SELECT * FROM photos");
        break;
    case VITA_OHFI_VIDEO:
        //query = QSqlQuery("SELECT * FROM videos");
        break;
    case VITA_OHFI_PSPSAVE:
        //query = QSqlQuery("SELECT * FROM savedata");
        break;
    case VITA_OHFI_VITAAPP:
    case VITA_OHFI_PSPAPP:
    case VITA_OHFI_PSXAPP:
    case VITA_OHFI_PSMAPP:
    case VITA_OHFI_BACKUP:
        query = QSqlQuery("SELECT "
                          "t0.object_id as ohfi,"
                          ":app_type as parent,"
                          "t3.path as path,"
                          "t0.title as name,"
                          "t1.type as type,"
                          "t1.data_type as data_type,"
                          "t3.size as size,"
                          "t3.date_created as date_created "
                          "FROM application t0 "
                          "JOIN object_node t1 on t0.object_id = t1.object_id "
                          "JOIN sources t3 ON t3.object_id = t0.object_id "
                          "WHERE app_type = :app_type");
        query.bindValue(0, root_ohfi);
        if(query.exec()) {
            metadata_t **last = &*metadata;
            while(query.next()) {
                metadata_t *meta = new metadata_t();
                fillMetadata(query, *meta);
                *last = meta;
                last = &meta->next_metadata;
                count++;
            }
        }
        break;
    default:
        qFatal("Invalid root ohfi type");
    }

    return count;
}

bool SQLiteDB::insertVirtualEntry(int ohfi)
{
    QSqlQuery query;
    query.prepare("REPLACE INTO virtual_nodes (object_id)"
                  "VALUES (:object_id)");
    query.bindValue(0, ohfi);
    bool ret = query.exec();
    if(!ret) {
        qDebug() << query.lastError();
    }
    return ret;
}

bool SQLiteDB::insertVirtualEntries()
{
    int ohfi;

    if((ohfi = insertNodeEntry("Folders", VITA_DIR_TYPE_MASK_REGULAR, Video)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("All", VITA_DIR_TYPE_MASK_ALL, Video)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Folders", VITA_DIR_TYPE_MASK_REGULAR, Photo)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Month", VITA_DIR_TYPE_MASK_MONTH, Photo)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("All", VITA_DIR_TYPE_MASK_ALL, Photo)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Artists", VITA_DIR_TYPE_MASK_ARTISTS, Music)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Albums", VITA_DIR_TYPE_MASK_ALBUMS, Music)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Songs", VITA_DIR_TYPE_MASK_SONGS, Music)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Genres", VITA_DIR_TYPE_MASK_GENRES, Music)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    if((ohfi = insertNodeEntry("Playlists", VITA_DIR_TYPE_MASK_PLAYLISTS, Music)) > 0) {
        insertVirtualEntry(ohfi);
    } else {
        return false;
    }

    return true;
}

bool SQLiteDB::getObjectList(int ohfi, metadata_t **metadata)
{
    Q_UNUSED(ohfi);
    Q_UNUSED(metadata);
    return false;
}
