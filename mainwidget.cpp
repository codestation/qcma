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
    startServer();
}

void MainWidget::dialogResult(int result)
{
    if(result == QDialog::Accepted) {
        if(first_run) {
            first_run = false;
            startServer();
        }
    } else if(first_run) {
        qApp->quit();
    }
}

void MainWidget::startServer()
{
    qDebug("Starting cma event loop");
    clientLoop.start();
}

void MainWidget::stopServer()
{
    clientLoop.stop();
}

void MainWidget::refreshDatabase()
{
    clientLoop.db.mutex.lock();
    clientLoop.db.destroy();
    int count = clientLoop.db.create();
    qDebug("Added %i entries to the database", count);
    clientLoop.db.mutex.unlock();
}

void MainWidget::deviceDisconnect()
{
    setTrayTooltip(tr("Disconnected"));
    receiveMessage(tr("The device has been disconnected"));
}

void MainWidget::connectSignals()
{
    connect(&dialog, SIGNAL(finished(int)), this, SLOT(dialogResult(int)));
    connect(&clientLoop, SIGNAL(receivedPin(int)), this, SLOT(showPin(int)));
    connect(&clientLoop, SIGNAL(deviceConnected(QString)), this, SLOT(receiveMessage(QString)));
    connect(&clientLoop, SIGNAL(deviceConnected(QString)), this, SLOT(setTrayTooltip(QString)));
    connect(&clientLoop, SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnect()));
    connect(&clientLoop, SIGNAL(refreshDatabase()), this, SLOT(refreshDatabase()));
    connect(&clientLoop, SIGNAL(finished()), qApp, SLOT(quit()));
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
    refreshDatabase();
}

void MainWidget::setTrayTooltip(QString message)
{
    trayIcon->setToolTip(message);
}

void MainWidget::toggleWireless()
{
    QSettings settings;
    if(wireless->isChecked()) {
        wireless->setText(tr("&Wireless enabled"));
        settings.setValue("wireless", true);
    } else {
        wireless->setText(tr("&Wireless disabled"));
        settings.setValue("wireless", false);
    }
}

void MainWidget::createTrayIcon()
{
    QSettings settings;
    wireless = new QAction(this);
    wireless->setCheckable(true);

    if(settings.value("wireless", false).toBool()) {
        wireless->setText(tr("&Wireless enabled"));
        wireless->setChecked(true);
    } else {
        wireless->setText(tr("&Wireless disabled"));
    }

    options = new QAction(tr("&Settings"), this);
    reload = new QAction(tr("&Refresh database"), this);
    quit = new QAction(tr("&Quit"), this);

    connect(options, SIGNAL(triggered()), &dialog, SLOT(open()));
    //connect(reload, SIGNAL(triggered()), &CmaWorker, SLOT(allowRefresh()), Qt::DirectConnection);
    connect(quit, SIGNAL(triggered()), this, SLOT(stopServer()));
    connect(wireless, SIGNAL(triggered()), this, SLOT(toggleWireless()));

    QMenu *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(options);
    trayIconMenu->addAction(reload);
    trayIconMenu->addAction(wireless);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/main/resources/psv_icon.png"));
    trayIcon->show();
    // try to avoid the iconTray Qt bug
    //Sleeper::sleep(1);
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
