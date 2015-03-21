/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2014  Codestation
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

#ifndef KDENOTIFIERTRAY_H
#define KDENOTIFIERTRAY_H

#include "trayindicator.h"
#include "kdenotifier.h"

class QAction;
class QSystemTrayIcon;

class KDENotifierTray : public TrayIndicator
{
    Q_OBJECT
public:
    explicit KDENotifierTray(QWidget *parent = 0);
    void init();
    void setIcon(const QString &icon);
    bool isVisible();
    void show();
    void hide();
    void showMessage(const QString &title, const QString &message);

private:
    //system tray
    QAction *quit;
    QAction *reload;
    QAction *options;
    QAction *backup;
    QAction *about;
    QAction *about_qt;

    KDENotifier *m_notifier_item;

public slots:

};

#endif // KDENOTIFIERTRAY_H
