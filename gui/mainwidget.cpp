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

#include "mainwidget.h"
#include "cmaclient.h"
#include "cmautils.h"

#include "qlistdb.h"
#include "sqlitedb.h"

#include "qtrayicon.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QLibrary>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSettings>
#include <QSpacerItem>

const QStringList MainWidget::path_list = QStringList() << "photoPath" << "musicPath" << "videoPath" << "appsPath" << "urlPath";

bool sleptOnce = false;

MainWidget::MainWidget(QWidget *obj_parent) :
    QWidget(obj_parent), db(NULL), configForm(NULL), managerForm(NULL), backupForm(NULL)
{
    trayIcon = NULL;
}

void MainWidget::checkSettings()
{
    QSettings settings;
    // make sure that all the paths are set, else show the config dialog
    foreach(const QString &path, path_list) {
        if(!settings.contains(path)) {
            first_run = true;
            configForm->show();
            return;
        }
    }
    first_run = false;
    managerForm->start();
}

void MainWidget::dialogResult(int result)
{
    if(result == QDialog::Accepted) {
        if(first_run) {
            first_run = false;
            managerForm->start();
        }
    } else if(first_run) {
        qApp->quit();
    }
}

void MainWidget::stopServer()
{
    setTrayTooltip(tr("Shutting down..."));
    if(CmaClient::isRunning()) {
        receiveMessage(tr("Stopping QCMA (disconnect your PS Vita)"));
    }
    managerForm->stop();
}

void MainWidget::deviceDisconnect()
{
#ifndef Q_OS_WIN32
    trayIcon->setIcon("qcma_off");
#else
    trayIcon->setIcon("tray/qcma_off_16");
#endif
    qDebug("Icon changed - disconnected");
    setTrayTooltip(tr("Disconnected"));
    receiveMessage(tr("The device has been disconnected"));
}

void MainWidget::deviceConnect(QString message)
{
#ifndef Q_OS_WIN32
    trayIcon->setIcon("qcma_on");
#else
    trayIcon->setIcon("tray/qcma_on_16");
#endif
    qDebug("Icon changed - connected");
    setTrayTooltip(message);
    receiveMessage(message);
}

void MainWidget::prepareApplication(bool showSystray)
{
    //TODO: delete database before exit
    if(QSettings().value("useMemoryStorage", true).toBool()) {
        db = new QListDB();
    } else {
        db = new SQLiteDB();
    }

    configForm = new ConfigWidget();
    backupForm = new BackupManagerForm(db, this);
    managerForm = new ClientManager(db, this);
    connectSignals();

    if(showSystray) {
        createTrayIcon();
    }

    checkSettings();
}

void MainWidget::connectSignals()
{
    connect(configForm, SIGNAL(finished(int)), this, SLOT(dialogResult(int)));
    connect(managerForm, SIGNAL(stopped()), qApp, SLOT(quit()));
    connect(managerForm, SIGNAL(deviceConnected(QString)), this, SIGNAL(deviceConnected(QString)));
    connect(managerForm, SIGNAL(deviceDisconnected()), this, SIGNAL(deviceDisconnected()));
    connect(managerForm, SIGNAL(messageSent(QString)), this, SIGNAL(messageReceived(QString)));
    connect(managerForm, SIGNAL(updated(int)), this, SIGNAL(databaseUpdated(int)));
}

void MainWidget::setTrayTooltip(QString message)
{
    if(trayIcon) {
        trayIcon->setToolTip(message);
    }
}

void MainWidget::openManager()
{
    backupForm->loadBackupListing(-1);
    backupForm->show();
}

void MainWidget::showAboutDialog()
{
    QMessageBox about;

    about.setText(QString("Qcma ") + QCMA_VER);
    about.setWindowTitle(tr("About Qcma"));
#ifndef QCMA_BUILD_HASH
    about.setInformativeText(tr("Copyright (C) 2016  Codestation") + "\n");
#else
    about.setInformativeText(tr("Copyright (C) 2016  Codestation\n\nbuild hash: %1\nbuild branch: %2").arg(QCMA_BUILD_HASH).arg(QCMA_BUILD_BRANCH));
#endif
    about.setStandardButtons(QMessageBox::Ok);
    about.setIconPixmap(QPixmap(":/main/resources/images/qcma.png"));
    about.setDefaultButton(QMessageBox::Ok);

    // hack to expand the messagebox minimum size
    QSpacerItem* horizontalSpacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* widget_layout = (QGridLayout*)about.layout();
    widget_layout->addItem(horizontalSpacer, widget_layout->rowCount(), 0, 1, widget_layout->columnCount());

    about.show();
    about.exec();
}

void MainWidget::showAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWidget::openConfig()
{
    configForm->open();
}

void MainWidget::refreshDatabase()
{
    managerForm->refreshDatabase();
}

TrayIndicator *MainWidget::createTrayObject(QWidget *obj_parent)
{
    TrayFunctionPointer create_tray = NULL;

#ifdef Q_OS_LINUX
    QString desktop = getenv("XDG_CURRENT_DESKTOP");
    qDebug() << "Current desktop: " << desktop;

#ifdef QT_DEBUG
    QString library_path = QApplication::applicationDirPath();
#else
    QString library_path = "/usr/lib/qcma";
#endif

    if(desktop.toLower() == "kde")
    {
#ifdef QT_DEBUG
        library_path += "/../kdenotifier";
#endif
        // KDENotifier
        QLibrary library(library_path + "/libqcma_kdenotifier.so");
        if(library.load())
            create_tray = reinterpret_cast<TrayFunctionPointer>(library.resolve("createTrayIndicator"));
        else
            qWarning() << "Cannot load libqcma_kdenotifier plugin from" << library_path;
    }
    else
    // try to use the appindicator if is available
    // if(desktop.toLower() == "unity")
    {
#ifdef QT_DEBUG
        library_path += "/../appindicator";
#endif
        // AppIndicator
        QLibrary library(library_path + "/libqcma_appindicator.so");
        if(library.load())
            create_tray = reinterpret_cast<TrayFunctionPointer>(library.resolve("createTrayIndicator"));
        else
            qWarning() << "Cannot load libqcma_appindicator plugin from" << library_path;
    }
#endif
    // else QSystemTrayIcon
    return (create_tray != NULL) ? create_tray(obj_parent) : createTrayIndicator(obj_parent);
}

void MainWidget::createTrayIcon()
{    
    trayIcon = createTrayObject(this);
    trayIcon->init();

#ifndef Q_OS_WIN32
    trayIcon->setIcon("qcma_off");
#else
    trayIcon->setIcon("tray/qcma_off_16");
#endif
    trayIcon->show();

    connect(trayIcon, SIGNAL(openConfig()), this, SLOT(openConfig()));
    connect(trayIcon, SIGNAL(openManager()), this, SLOT(openManager()));
    connect(trayIcon, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));
    connect(trayIcon, SIGNAL(showAboutDialog()), this, SLOT(showAboutDialog()));
    connect(trayIcon, SIGNAL(showAboutQt()), this, SLOT(showAboutQt()));
    connect(trayIcon, SIGNAL(stopServer()), this, SLOT(stopServer()));

    connect(managerForm, SIGNAL(deviceConnected(QString)), this, SLOT(deviceConnect(QString)));
    connect(managerForm, SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnect()));
    connect(managerForm, SIGNAL(messageSent(QString)), this, SLOT(receiveMessage(QString)));
}

void MainWidget::receiveMessage(QString message)
{
    // a timeout is added before the popups are displayed to prevent them from
    // appearing in the wrong location
    if(!sleptOnce) {
        Sleeper::sleep(1);
        sleptOnce = true;
    }

    if(trayIcon->isVisible())
        trayIcon->showMessage(tr("Information"), message);
}

MainWidget::~MainWidget()
{
    if(trayIcon) {
        trayIcon->hide();
    }
    delete db;
}
