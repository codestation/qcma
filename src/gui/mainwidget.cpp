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

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QSettings>
#include <QSpacerItem>

#ifdef ENABLE_KDE_NOTIFIER
#include <kmenu.h>
#endif

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
#ifndef ENABLE_KDE_NOTIFIER
    trayIcon = NULL;
#else
    notifierItem = NULL;
#endif
}
#else
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent), db(NULL), configForm(NULL), managerForm(NULL), backupForm(NULL)
{
#ifndef ENABLE_KDE_NOTIFIER
    trayIcon = NULL;
#else
    notifierItem = NULL;
#endif
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
#ifndef ENABLE_KDE_NOTIFIER
#ifndef Q_OS_WIN32
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon_dc.png"));
#else
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon_16_dc.png"));
#endif
#else
    notifierItem->setIconByPixmap(QIcon(":/main/resources/images/psv_icon_dc.png"));
#endif
    qDebug("Icon changed - disconnected");
    setTrayTooltip(tr("Disconnected"));
    receiveMessage(tr("The device has been disconnected"));
}

void MainWidget::deviceConnect(QString message)
{
#ifndef ENABLE_KDE_NOTIFIER
#ifndef Q_OS_WIN32
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon.png"));
#else
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon_16.png"));
#endif
#else
    notifierItem->setIconByPixmap(QIcon(":/main/resources/images/psv_icon.png"));
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

    configForm = new ConfigWidget(this);
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

void MainWidget::createTrayIcon()
{
    options = new QAction(tr("&Settings"), this);
    reload = new QAction(tr("&Refresh database"), this);
    backup = new QAction(tr("&Backup Manager"), this);
    about = new QAction(tr("&About QCMA"), this);
    about_qt = new QAction(tr("Abou&t Qt"), this);
    quit = new QAction(tr("&Quit"), this);

    connect(options, SIGNAL(triggered()), this, SLOT(openConfig()));
    connect(backup, SIGNAL(triggered()), this, SLOT(openManager()));
    connect(reload, SIGNAL(triggered()), this, SLOT(refreshDatabase()));
    connect(about, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
    connect(about_qt, SIGNAL(triggered()), this, SLOT(showAboutQt()));
    connect(quit, SIGNAL(triggered()), this, SLOT(stopServer()));

#ifndef ENABLE_KDE_NOTIFIER
    QMenu *trayIconMenu = new QMenu(this);
#else
    KMenu *trayIconMenu = new KMenu(this);
#endif

    trayIconMenu->addAction(options);
    trayIconMenu->addAction(reload);
    trayIconMenu->addAction(backup);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(about);
    trayIconMenu->addAction(about_qt);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit);

#ifndef ENABLE_KDE_NOTIFIER
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
#ifndef Q_OS_WIN32
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon_dc.png"));
#else
    trayIcon->setIcon(QIcon(":/main/resources/images/psv_icon_16_dc.png"));
#endif
    trayIcon->show();
#else
    notifierItem = new KDENotifier("QcmaNotifier", this);
    notifierItem->setContextMenu(trayIconMenu);
    notifierItem->setTitle("Qcma");
    notifierItem->setCategory(KStatusNotifierItem::ApplicationStatus);
    notifierItem->setIconByPixmap(QIcon(":/main/resources/images/psv_icon_dc.png"));
    notifierItem->setStatus(KStatusNotifierItem::Active);
    notifierItem->setToolTipTitle(tr("Qcma status"));
    notifierItem->setToolTipIconByPixmap(QIcon(":/main/resources/images/qcma.png"));
    notifierItem->setToolTipSubTitle(tr("Disconnected"));
    notifierItem->setStandardActionsEnabled(false);
#endif

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

#ifndef ENABLE_KDE_NOTIFIER
    if(trayIcon->isVisible()) {
        trayIcon->showMessage(tr("Information"), message);
    }
#else
    notifierItem->showMessage(tr("Qcma - Information"), message, "dialog-information", 3000);
#endif

}

MainWidget::~MainWidget()
{
#ifndef ENABLE_KDE_NOTIFIER
    if(trayIcon) {
        trayIcon->hide();
    }
#endif
    delete db;
}
