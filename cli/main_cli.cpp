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

#ifndef Q_OS_WIN32
#include <signal.h>
#endif

#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QStringList>
#include <QTextCodec>
#include <QThread>
#include <QTranslator>

#include <inttypes.h>
#include <vitamtp.h>

#include "singlecoreapplication.h"
#include "headlessmanager.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
static void noDebugOutput(QtMsgType type, const QMessageLogContext &, const QString & msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *message = localMsg.constData();
#else
static void noDebugOutput(QtMsgType type, const char *message)
{
#endif
    switch (type) {
    case QtDebugMsg:
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", message);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", message);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", message);
        abort();
        break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg:
        fprintf(stderr, "Info: %s\n", message);
        break;
#endif
    }
}

static bool setup_handlers()
{
    struct sigaction hup, term;

    memset(&hup, 0, sizeof(hup));
    memset(&term, 0, sizeof(term));

    hup.sa_handler = HeadlessManager::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hup, NULL) != 0) {
        qCritical("SIGHUP signal handle failed");
        return false;
    }

    term.sa_handler = HeadlessManager::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, NULL) != 0) {
        qCritical("SIGTERM signal handle failed");
        return false;
    }

    if (sigaction(SIGINT, &term, NULL) != 0) {
        qCritical("SIGINT signal handle failed");
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(commonrc);
    Q_INIT_RESOURCE(translations);

    if(SingleCoreApplication::sendMessage("Another instance of Qcma tried to start")) {
        QTextStream(stdout) << "An instance of Qcma is already running" << endl;
        return 0;
    }

    SingleCoreApplication app(argc, argv);

    // FIXME: libmtp sends SIGPIPE if a socket write fails crashing the whole app
    // the proper fix is to libmtp to handle the cancel properly or ignoring
    // SIGPIPE on the socket
    signal(SIGPIPE, SIG_IGN);

    if(!setup_handlers())
        return 1;

    if(app.arguments().contains("--with-debug")) {
        VitaMTP_Set_Logging(VitaMTP_DEBUG);
    } else if(app.arguments().contains("--verbose")) {
        VitaMTP_Set_Logging(VitaMTP_VERBOSE);
    } else {
        VitaMTP_Set_Logging(VitaMTP_NONE);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        qInstallMessageHandler(noDebugOutput);
#else
        qInstallMsgHandler(noDebugOutput);
#endif
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

    QTextStream(stdout) << "Starting Qcma " << QCMA_VER << endl;

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
    app.setOrganizationName("codestation");
    app.setApplicationName("qcma");

    HeadlessManager manager;

    // receive the message from another process
    QObject::connect(&app, SIGNAL(messageAvailable(QString)), &manager, SLOT(receiveMessage(QString)));

    manager.start();

    return app.exec();
}
