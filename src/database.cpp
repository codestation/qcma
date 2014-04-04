#include "database.h"
#include "avdecoder.h"

#include <QImage>
#include <QSettings>
#include <QThread>

const file_type audio_list[] = {
    {"mp3", FILE_FORMAT_MP3, CODEC_TYPE_MP3},
    {"mp4", FILE_FORMAT_MP4, CODEC_TYPE_AAC},
    {"wav", FILE_FORMAT_WAV, CODEC_TYPE_PCM}
};

const file_type photo_list[] = {
    {"jpg",  FILE_FORMAT_JPG, CODEC_TYPE_JPG},
    {"jpeg", FILE_FORMAT_JPG, CODEC_TYPE_JPG},
    {"png",  FILE_FORMAT_PNG, CODEC_TYPE_PNG},
    {"tif",  FILE_FORMAT_TIF, CODEC_TYPE_TIF},
    {"tiff", FILE_FORMAT_TIF, CODEC_TYPE_TIF},
    {"bmp",  FILE_FORMAT_BMP, CODEC_TYPE_BMP},
    {"gif",  FILE_FORMAT_GIF, CODEC_TYPE_GIF},
};

const file_type video_list[] = {
    {"mp4", FILE_FORMAT_MP4, 0}
};

Database::Database(QObject *parent) :
    QObject(parent),
    mutex(QMutex::Recursive)
{
}
#include <QDebug>
void Database::process()
{
    qDebug("Starting database_thread: 0x%016" PRIxPTR, (uintptr_t)QThread::currentThreadId());
    clear();
    cancel_operation = false;
    int count = create();
    cancel_operation = false;
    qDebug("Total entries added to the database: %i", count);
    if(count < 0) {
        clear();
    }
    emit updated(count);
    mutex.unlock();
}

void Database::cancelOperation()
{
    QMutexLocker locker(&cancel);
    cancel_operation = true;
}

bool Database::continueOperation()
{
    QMutexLocker locker(&cancel);
    return !cancel_operation;
}

int Database::checkFileType(const QString path, int ohfi_root)
{
    switch(ohfi_root) {
    case VITA_OHFI_MUSIC:
        for(int i = 0, max = sizeof(audio_list) / sizeof(file_type); i < max; i++) {
            if(path.endsWith(audio_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    case VITA_OHFI_PHOTO:
        for(int i = 0, max = sizeof(photo_list) / sizeof(file_type); i< max; i++) {
            if(path.endsWith(photo_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    case VITA_OHFI_VIDEO:
        for(int i = 0, max = sizeof(video_list) / sizeof(file_type); i< max; i++) {
            if(path.endsWith(video_list[i].file_ext, Qt::CaseInsensitive)) {
                return i;
            }
        }
        break;
    default:
        return 0;
    }
    return -1;
}

void Database::loadMusicMetadata(const QString &path, metadata_t &metadata)
{
    AVDecoder decoder;
    bool skipMetadata = QSettings().value("skipMetadata", false).toBool();

    if(!skipMetadata && decoder.open(path)) {
        decoder.getAudioMetadata(metadata);
    } else {
        metadata.data.music.album = strdup("");
        metadata.data.music.artist = strdup("");
        metadata.data.music.title = strdup(metadata.name);
    }
}

void Database::loadVideoMetadata(const QString &path, metadata_t &metadata)
{
    AVDecoder decoder;
    bool skipMetadata = QSettings().value("skipMetadata", false).toBool();

    if(!skipMetadata && decoder.open(path)) {
        decoder.getVideoMetadata(metadata);
    } else {
        metadata.data.video.title = strdup(metadata.name);
        metadata.data.video.explanation = strdup("");
        metadata.data.video.copyright = strdup("");
        // default to H264 video codec
        metadata.data.video.tracks->data.track_video.codecType = CODEC_TYPE_AVC;
    }
}

void Database::loadPhotoMetadata(const QString &path, metadata_t &metadata)
{
    QImage img;
    bool skipMetadata = QSettings().value("skipMetadata", false).toBool();

    if(!skipMetadata && img.load(path)) {
        metadata.data.photo.tracks->data.track_photo.width = img.width();
        metadata.data.photo.tracks->data.track_photo.height = img.height();
    }
    metadata.data.photo.title = strdup(metadata.name);
}
