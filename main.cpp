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

#include <QDebug>
#include <QThread>

#include "singleapplication.h"
#include "mainwidget.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void noMessageOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    const char * msg = str.toStdString().c_str();
#else
void noMessageOutput(QtMsgType type, const char *msg)
{
#endif
     Q_UNUSED(type);
     Q_UNUSED(msg);
}

int main(int argc, char *argv[])
{
    if(SingleApplication::sendMessage(QObject::tr("A instance of QCMA is already running"))) {
        return 0;
    }

    SingleApplication app(argc, argv);



    if(!app.arguments().contains("--with-debug")) {
        VitaMTP_Set_Logging(VitaMTP_NONE);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        qInstallMessageHandler(noMessageOutput);
#else
        qInstallMsgHandler(noMessageOutput);
#endif
    } else {
        VitaMTP_Set_Logging(VitaMTP_DEBUG);
    }

    qDebug()<<"From main thread: "<<QThread::currentThreadId();

    // set the organization/application for QSettings to work properly
    app.setOrganizationName("qcma");
    app.setApplicationName("qcma");

    //TODO: check if this is actually needed since we don't have a main window by default
    QApplication::setQuitOnLastWindowClosed(false);

    MainWidget widget;
    widget.prepareApplication();

    // receive the message from another process
    QObject::connect(&app, SIGNAL(messageAvailable(QString)), &widget, SLOT(receiveMessage(QString)));

    return app.exec();
}
