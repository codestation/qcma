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

#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QFile>
#include <QStringList>
#include <QTextCodec>
#include <QThread>
#include <QTranslator>
#include <QStandardPaths>

#include <inttypes.h>
#include <signal.h>
#include <unistd.h>

#include <vitamtp.h>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

#include "servicemanager.h"

static QString getAppName()
{
    pid_t pid = getpid();
    QString cmdline = QString("/proc/%1/cmdline").arg(pid);
    QFile file(cmdline);
    return file.open(QIODevice::ReadOnly) ? file.readLine() : "";
}

#ifdef Q_OS_ANDROID
static void cleanOutput(QtMsgType type, const QMessageLogContext &, const QString & msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *message = localMsg.constData();

    switch (type) {
    case QtDebugMsg:
        __android_log_print(ANDROID_LOG_DEBUG, "qcma", "%s", message);
        break;
    case QtWarningMsg:
        __android_log_print(ANDROID_LOG_WARN, "qcma", "%s", message);
        break;
    case QtCriticalMsg:
        __android_log_print(ANDROID_LOG_ERROR, "qcma", "%s", message);
        break;
    case QtFatalMsg:
        __android_log_print(ANDROID_LOG_FATAL, "qcma", "%s", message);
        abort();
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg:
        __android_log_print(ANDROID_LOG_INFO, "qcma", "%s", message);
        break;
#endif
    }
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("qcma");

    // FIXME: libmtp sends SIGPIPE if a socket write fails crashing the whole app
    // the proper fix is to libmtp to handle the cancel properly or ignoring
    // SIGPIPE on the socket
    signal(SIGPIPE, SIG_IGN);

    // stdout goes to /dev/null on android
    VitaMTP_Set_Logging(VitaMTP_NONE);

#ifdef Q_OS_ANDROID
    qInstallMessageHandler(cleanOutput);
#endif

    qDebug("Starting Qcma %s", QCMA_VER);

    QString appname = getAppName();
    qDebug("Class name: %s", qPrintable(appname));

    app.addLibraryPath(appname + "/lib");

    QTranslator translator;
    QString locale = QLocale().system().name();
    qDebug() << "Current locale:" << locale;

    if(app.arguments().contains("--set-locale")) {
        int index = app.arguments().indexOf("--set-locale");
        if(index + 1 < app.arguments().length()) {
            qDebug("Enforcing locale: %s", app.arguments().at(index + 1).toUtf8().data());
            locale = app.arguments().at(index + 1);
        }
    }

    if(translator.load("qcma_" + locale, ":/resources/translations")) {
        app.installTranslator(&translator);
    } else {
        qWarning() << "Cannot load translation for locale:" << locale;
    }

    QTranslator system_translator;
    system_translator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&system_translator);

    qDebug("Starting main thread: 0x%016" PRIxPTR, (uintptr_t)QThread::currentThreadId());

    // set the organization/application for QSettings to work properly
    app.setOrganizationName("qcma");
    app.setApplicationName("qcma");

    ServiceManager manager;
    manager.start();

    return app.exec();
}
