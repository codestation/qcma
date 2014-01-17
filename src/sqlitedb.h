#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <vitamtp.h>

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>

#define OBJECT_FILE 0x10000000
#define OBJECT_FOLDER 0x20000000
#define OBJECT_SPECIAL 0x40000000

#define OBJECT_MUSIC 0x00000100
#define OBJECT_PHOTO 0x00000200
#define OBJECT_VIDEO 0x00000400

#define OBJECT_ALBUM 0x00000002
#define OBJECT_ARTIST 0x00000005
#define OBJECT_ALBUM_ARTIST 0x00000008
#define OBJECT_GENRE 0x0000000B

#define OBJECT_SAVEDATA 0x00040000
#define OBJECT_SAVEDATA_FILE 0x00000002

class SQLiteDB : public QObject
{
    Q_OBJECT
public:
    explicit SQLiteDB(QObject *parent = 0);
    ~SQLiteDB();

    bool open();
    int create();
    void remove();
    bool initialize();
    QSqlError getLastError();

    int getPathId(const QString &path);
    QString getPathFromId(int ohfi);
    bool updateSize(int ohfi, quint64 size);
    bool deleteEntry(int ohfi);
    bool deleteEntry(const QString &path);
    uint insertObjectEntry(const char *title, int type);
    bool insertSourceEntry(uint object_id, const QString &path);
    uint insertMusicEntry(const QString &path, int type, int parent);
    uint insertVideoEntry(const QString &path, int type, int parent);
    uint insertPhotoEntry(const QString &path, int type, int parent);
    uint insertSavedataEntry(const QString &path, int type, int parent);

private:    
    int recursiveScanRootDirectory(const QString &base_path, int parent, int type);
    uint insertDirectoryEntry(const QString &path, int type, int parent);
    int checkFileType(const QString path, int ohfi_root);
    bool updateAdjacencyList(int ohfi, int parent);

    QString uuid;
    QSqlDatabase db;

signals:

public slots:

};

#endif // SQLITEDB_H
