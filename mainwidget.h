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

#include "configwidget.h"
#include "cmaserver.h"
#include "database.h"

#include <QAction>
#include <QWidget>
#include <QSystemTrayIcon>

extern "C" {
#include <vitamtp.h>
}


class QMenu;

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

    void prepareApplication();

private:
    void createTrayIcon();
    void checkSettings();
    void connectSignals();

    bool first_run;
    ConfigWidget dialog;
    QSystemTrayIcon *trayIcon;
    QAction *quit;
    QAction *reload;
    QAction *options;
    QAction *wireless;
    Database db;
    CmaServer server;
    const static QStringList path_list;

private slots:
    void deviceDisconnect();
    void dialogResult(int result);
    void receiveMessage(QString message);
    void setTrayTooltip(QString message);
    void toggleWireless();
    void showPin(int pin);
    void startClient(vita_device_t *device);
    void refreshDatabase();
    void startServer();
};

#endif // MAINWIDGET_H
