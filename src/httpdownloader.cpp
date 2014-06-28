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

char *HTTPDownloader::buffer = NULL;
qint64 HTTPDownloader::bufferSize = 0;
qint64 HTTPDownloader::downloadLeft = 0;

HTTPDownloader::HTTPDownloader(const QString &url, QObject *parent) :
    QObject(parent), remote_url(url), firstRead(true)
{
    lengthMutex.lock();
}

HTTPDownloader::~HTTPDownloader()
{
    lengthMutex.unlock();
    dataAvailable.unlock();
    free(buffer);
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
        downloadLeft = m_contentLength + 8;
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
    dataRead.lock();

    int currOffset = bufferSize;

    if(bufferSize == 0) {
        bufferSize = reply->bytesAvailable();

        if(firstRead) {
            bufferSize += 8;
            currOffset += 8;

            // start with a 16KiB buffer
            buffer = (char *)malloc(16384);
            *(uint64_t *)buffer = m_contentLength;
            firstRead = false;
        }

    } else {
        bufferSize += reply->bytesAvailable();

        if(bufferSize > 16384) {
            buffer = (char *)realloc(buffer, bufferSize);
        }
    }

    reply->read(buffer + currOffset, reply->bytesAvailable());
    downloadLeft -= bufferSize;

    if(bufferSize >= 16384 || downloadLeft == 0) {
        dataAvailable.unlock();
    }
    dataRead.unlock();
}

int HTTPDownloader::readCallback(unsigned char *data, unsigned long wantlen, unsigned long *gotlen)
{
    if(!dataAvailable.tryLock(30000)) {
        qWarning("Connection timeout while receiving data from network, aborting");
        return -1;
    }

    dataRead.lock();

    if(bufferSize == 0) {
        dataRead.unlock();
        return -1;
    }

    if(bufferSize < wantlen) {
        wantlen = bufferSize;
    }

    memcpy(data, buffer, wantlen);
    bufferSize -= wantlen;
    *gotlen = wantlen;

    if(bufferSize > 0) {
        memmove(buffer, buffer + wantlen, bufferSize);
        if(bufferSize >= 16384 || downloadLeft == 0) {
            dataAvailable.unlock();
        }
    }

    *gotlen = wantlen;
    dataRead.unlock();

    return PTP_RC_OK;
}

void HTTPDownloader::error(QNetworkReply::NetworkError errorCode)
{
    Q_UNUSED(errorCode);
    QString error = reply->errorString();

    qWarning() << "Network error:" << error;
    emit messageSent(tr("Network error: %1").arg(error));

    // set buffer to zero so a read callback can be aborted
    dataRead.lock();
    bufferSize = 0;
    dataRead.unlock();
    lengthMutex.unlock();
}
