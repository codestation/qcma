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
#include "clientmanager.h"
#include "cmaclient.h"

#include <QAction>
#include <QWidget>
#include <QSystemTrayIcon>

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
    ConfigWidget dialog;
    QSystemTrayIcon *trayIcon;
    QAction *quit;
    QAction *reload;
    QAction *options;
    ClientManager manager;
    const static QStringList path_list;

private slots:
    void deviceDisconnect();
    void dialogResult(int result);
    void receiveMessage(QString message);
    void setTrayTooltip(QString message);
    void showPin(int pin);
    void stopServer();
};

#endif // MAINWIDGET_H
