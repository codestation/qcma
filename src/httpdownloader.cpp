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

#include "httpdownloader.h"

#include <QDebug>
#include <QThread>

#include <vitamtp.h>

QNetworkReply *HTTPDownloader::reply = NULL;
volatile qint64 HTTPDownloader::m_contentLength = -1;

QMutex HTTPDownloader::dataAvailable;
QMutex HTTPDownloader::dataRead;

QByteArray HTTPDownloader::buffer;
bool HTTPDownloader::bufferReady = false;
qint64 HTTPDownloader::downloadLeft = 0;

HTTPDownloader::HTTPDownloader(const QString &url, QObject *obj_parent) :
    QObject(obj_parent), remote_url(url), firstRead(true)
{
    lengthMutex.lock();
}

HTTPDownloader::~HTTPDownloader()
{
    lengthMutex.unlock();
    dataAvailable.unlock();
}

void HTTPDownloader::downloadFile()
{
    dataAvailable.lock();
    qDebug("Starting http_thread: 0x%016" PRIxPTR, (uintptr_t)QThread::currentThreadId());
    request = new QNetworkAccessManager(this);
    reply = request->get(QNetworkRequest(QUrl(remote_url)));
    connect(reply, SIGNAL(metaDataChanged()), this, SLOT(metadataChanged()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    connect(reply, SIGNAL(finished()), request, SLOT(deleteLater()));
}

void HTTPDownloader::metadataChanged()
{
    QVariant len = reply->header(QNetworkRequest::ContentLengthHeader);
    if(len.isValid()) {
        m_contentLength = len.toInt();
        buffer.resize(8);
        *(uint64_t *)buffer.data() = m_contentLength;
        downloadLeft = m_contentLength;
    } else {
        m_contentLength = -1;
    }
    lengthMutex.unlock();
}

qint64 HTTPDownloader::getFileSize()
{
    lengthMutex.lock();
    return m_contentLength;
}

void HTTPDownloader::readyRead()
{
    QMutexLocker locker(&dataRead);

    downloadLeft -= reply->bytesAvailable();

    buffer.append(reply->readAll());

    if(buffer.size() >= 16 * 1024 || downloadLeft == 0) {
        dataAvailable.unlock();
    }

    if(downloadLeft == 0)
        qDebug() << "remote download complete";
}

int HTTPDownloader::readCallback(unsigned char *data, unsigned long wantlen, unsigned long *gotlen)
{
    if(downloadLeft && !dataAvailable.tryLock(30000)) {
        qWarning("Connection timeout while receiving data from network, aborting");
        return -1;
    }

    QMutexLocker locker(&dataRead);

    if(buffer.size() == 0)
        return -1;

    int read_size = wantlen > (unsigned long)buffer.size() ? buffer.size() : wantlen;

    memcpy(data, buffer.data(), read_size);
    buffer.remove(0, read_size);

    qDebug() << "sending data: " << read_size << ", left in buffer: " << buffer.size();

    *gotlen = read_size;

    return PTP_RC_OK;
}

void HTTPDownloader::error(QNetworkReply::NetworkError errorCode)
{
    Q_UNUSED(errorCode);
    QString str_error = reply->errorString();

    qWarning() << "Network error:" << str_error;
    emit messageSent(tr("Network error: %1").arg(str_error));

    lengthMutex.unlock();

    // clear the buffer so a read callback can be aborted
    QMutexLocker locker(&dataRead);
    buffer.clear();
}
