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
#include "cmaevent.h"
#include "utils.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QSettings>

const QStringList MainWidget::path_list = QStringList() << "photoPath" << "musicPath" << "videoPath" << "appsPath" << "urlPath";

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent)
{
}

void MainWidget::checkSettings()
{
    QSettings settings;
    // make sure that all the paths are set, else show the config dialog
    foreach(const QString &path, path_list) {
        if(!settings.contains(path)) {
            first_run = true;
            dialog.show();
            return;
        }
    }
    first_run = false;
    manager.start();
}

void MainWidget::dialogResult(int result)
{
    if(result == QDialog::Accepted) {
        if(first_run) {
            first_run = false;
            manager.start();
        }
    } else if(first_run) {
        qApp->quit();
    }
}

void MainWidget::stopServer()
{
    setTrayTooltip(tr("Shutting down..."));
    receiveMessage(tr("Stopping QCMA..."));
    manager.stop();
}

void MainWidget::deviceDisconnect()
{
    setTrayTooltip(tr("Disconnected"));
    receiveMessage(tr("The device has been disconnected"));
}

void MainWidget::showPin(int pin)
{
    receiveMessage(QString(tr("Received PIN: %1").arg(QString::number(pin), 8, QChar('0'))));
}

void MainWidget::prepareApplication()
{
    connectSignals();
    createTrayIcon();
    checkSettings();
}

void MainWidget::connectSignals()
{
    connect(&dialog, SIGNAL(finished(int)), this, SLOT(dialogResult(int)));
    connect(&manager, SIGNAL(stopped()), qApp, SLOT(quit()));
    connect(&manager, SIGNAL(receivedPin(int)), this, SLOT(showPin(int)));
    connect(&manager, SIGNAL(databaseUpdated(QString)), this, SLOT(receiveMessage(QString)));
    connect(&manager, SIGNAL(deviceConnected(QString)), this, SLOT(receiveMessage(QString)));
    connect(&manager, SIGNAL(deviceConnected(QString)), this, SLOT(setTrayTooltip(QString)));
    connect(&manager, SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnect()));
}

void MainWidget::setTrayTooltip(QString message)
{
    trayIcon->setToolTip(message);
}

void MainWidget::createTrayIcon()
{
    options = new QAction(tr("&Settings"), this);
    reload = new QAction(tr("&Refresh database"), this);
    quit = new QAction(tr("&Quit"), this);

    connect(options, SIGNAL(triggered()), &dialog, SLOT(open()));
    connect(reload, SIGNAL(triggered()), &manager, SLOT(refreshDatabase()));
    connect(quit, SIGNAL(triggered()), this, SLOT(stopServer()));

    QMenu *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(options);
    trayIconMenu->addAction(reload);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/main/resources/psv_icon.png"));
    trayIcon->show();
    // try to avoid the iconTray Qt bug
    Sleeper::sleep(1);
}

void MainWidget::receiveMessage(QString message)
{
    if(trayIcon->isVisible()) {
        trayIcon->showMessage(tr("Information"), message);
    }
}

MainWidget::~MainWidget()
{
    trayIcon->hide();
}
