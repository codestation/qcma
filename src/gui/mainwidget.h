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

#include <vitamtp.h>

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

    void prepareApplication();

private:
    void connectSignals();
    void createTrayIcon();
    void checkSettings();

    bool first_run;

    // database
    Database *db;

    // forms
    ConfigWidget *configForm;
    ClientManager *managerForm;
    BackupManagerForm *backupForm;

    //system tray
    QAction *quit;
    QAction *reload;
    QAction *options;
    QAction *backup;
    QAction *about;
    QAction *about_qt;

#ifndef ENABLE_KDE_NOTIFIER
    QSystemTrayIcon *trayIcon;
#else
    KDENotifier *notifierItem;
#endif

    const static QStringList path_list;

private slots:
    void stopServer();
    void openManager();
    void showAboutQt();
    void showAboutDialog();
    void deviceConnected(QString message);
    void deviceDisconnect();
    void dialogResult(int result);
    void receiveMessage(QString message);
    void setTrayTooltip(QString message);
};

#endif // MAINWIDGET_H
