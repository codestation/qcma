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

#ifdef Q_OS_LINUX
#include "clientmanager_adaptor.h"
#endif

#include "qlistdb.h"
#include "sqlitedb.h"

#include "indicator/qtrayicon.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSettings>
#include <QSpacerItem>

const QStringList MainWidget::path_list = QStringList() << "photoPath" << "musicPath" << "videoPath" << "appsPath" << "urlPath";

bool sleptOnce = false;

#ifdef Q_OS_LINUX
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent), db(NULL), configForm(NULL), managerForm(NULL), backupForm(NULL), dbus_conn(QDBusConnection::sessionBus())
{
    new ClientManagerAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    // expose qcma over dbus so the database update can be triggered
    dbus.registerObject("/ClientManager", this);
    dbus.registerService("org.qcma.ClientManager");
    trayIcon = NULL;
}
#else
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent), db(NULL), configForm(NULL), managerForm(NULL), backupForm(NULL)
{
    trayIcon = NULL;
}
#endif

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
#ifndef ENABLE_KDE_NOTIFIER
    if(trayIcon) {
        trayIcon->setToolTip(message);
    }
#else
    if(notifierItem) {
        notifierItem->setToolTipSubTitle(message);
    }
#endif
}

void MainWidget::openManager()
{
    backupForm->loadBackupListing(-1);
    backupForm->show();
}

void MainWidget::showAboutDialog()
{
    QMessageBox about;

    about.setText(QString("QCMA ") + QCMA_VER);
    about.setWindowTitle(tr("About QCMA"));
#ifndef QCMA_BUILD_HASH
    about.setInformativeText(tr("Copyright (C) 2014  Codestation") + "\n");
#else
    about.setInformativeText(tr("Copyright (C) 2014  Codestation\n\nbuild hash: %1\nbuild branch: %2").arg(QCMA_BUILD_HASH).arg(QCMA_BUILD_BRANCH));
#endif
    about.setStandardButtons(QMessageBox::Ok);
    about.setIconPixmap(QPixmap(":/main/resources/images/qcma.png"));
    about.setDefaultButton(QMessageBox::Ok);

    // hack to expand the messagebox minimum size
    QSpacerItem* horizontalSpacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)about.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

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

TrayIndicator *MainWidget::createTrayObject(QWidget *parent)
{
    TrayFunctionPointer create_tray = NULL;
#ifdef Q_OS_LINUX
    QString desktop = getenv("XDG_CURRENT_DESKTOP");
    qDebug() << "Current desktop: " << desktop;

    if(desktop.toLower() == "kde")
    {
        // KDENotifier
        QLibrary library("/usr/lib/qcma/libqcma_kdenotifier.so");
        if(library.load())
            create_tray = reinterpret_cast<TrayFunctionPointer>(library.resolve("createTrayIndicator"));
        else
            qDebug() << "Cannot load libqcma_kdenotifier plugin";
    }
    else
    // try to use the appindicator if is available
    // if(desktop.toLower() == "unity")
    {
        // AppIndicator
        QLibrary library("/usr/lib/qcma/libqcma_appindicator.so");
        if(library.load())
            create_tray = reinterpret_cast<TrayFunctionPointer>(library.resolve("createTrayIndicator"));
        else
            qDebug() << "Cannot load libqcma_appindicator plugin";
    }
#endif
    // else QSystemTrayIcon
    return (create_tray != NULL) ? create_tray(parent) : createTrayIndicator(parent);
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
