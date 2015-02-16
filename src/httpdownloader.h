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

#ifndef HTTPDOWNLOADER_H
#define HTTPDOWNLOADER_H

#include <QByteArray>
#include <QMutex>
#include <QNetworkReply>
#include <QObject>
#include <QWaitCondition>

#include <inttypes.h>

class HTTPDownloader : public QObject
{
    Q_OBJECT
public:
    explicit HTTPDownloader(const QString &url, QObject *parent = 0);
    ~HTTPDownloader();
    qint64 getFileSize();

signals:
    void messageSent(QString);

public slots:
    void downloadFile();
    static int readCallback(unsigned char *data, unsigned long wantlen, unsigned long *gotlen);
    void metadataChanged();
    void readyRead();
    void error(QNetworkReply::NetworkError);

private:
    QString remote_url;
    QNetworkAccessManager *request;
    QMutex lengthMutex;
    bool firstRead;

    static QMutex dataAvailable;
    static QMutex dataRead;

    static QNetworkReply *reply;
    volatile static qint64 m_contentLength;

    static QByteArray buffer;
    static bool bufferReady;
    static qint64 downloadLeft;
};

#endif // HTTPDOWNLOADER_H
