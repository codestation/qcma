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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "cmaclient.h"
#include "clientmanager.h"
#include "forms/configwidget.h"
#include "forms/backupmanagerform.h"
#include "forms/progressform.h"

#include <QAction>
#include <QWidget>

#include <QSystemTrayIcon>
#ifdef ENABLE_KDE_NOTIFIER
#include "kdenotifier.h"
#endif

#ifdef Q_OS_LINUX
#include <QDBusConnection>
#endif

#include <vitamtp.h>

class TrayIndicator;

class MainWidget : public QWidget
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qcma.ClientManager")

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

    void prepareApplication(bool showSystray);

private:
    void connectSignals();
    void createTrayIcon();
    void checkSettings();
    TrayIndicator *createTrayObject(QWidget *parent);

    bool first_run;

    // database
    Database *db;

    // forms
    ConfigWidget *configForm;
    ClientManager *managerForm;
    BackupManagerForm *backupForm;

#ifdef Q_OS_LINUX
    QDBusConnection dbus_conn;
#endif

    TrayIndicator *trayIcon;

    const static QStringList path_list;

signals:
    Q_SCRIPTABLE void deviceConnected(QString);
    Q_SCRIPTABLE void deviceDisconnected();
    Q_SCRIPTABLE void databaseUpdated(int count);
    Q_SCRIPTABLE void messageReceived(QString message);

public slots:
    void openConfig();
    void openManager();
    void showAboutQt();
    void showAboutDialog();
    void refreshDatabase();
    void stopServer();

private slots:
    void deviceConnect(QString message);
    void deviceDisconnect();
    void dialogResult(int result);
    void receiveMessage(QString message);
    void setTrayTooltip(QString message);
};

#endif // MAINWIDGET_H
