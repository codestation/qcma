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

#ifndef QTRAYICON_H
#define QTRAYICON_H

#include "trayindicator.h"

class QAction;
class QSystemTrayIcon;

class QTrayIcon : public TrayIndicator
{
    Q_OBJECT
public:
    explicit QTrayIcon(QWidget *parent = 0);
    ~QTrayIcon();
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

    QSystemTrayIcon *m_tray_icon;
};

#endif // QTRAYICON_H
