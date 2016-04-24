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

#include "qtrayicon.h"

#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

#ifdef Q_OS_LINUX
#undef signals
extern "C" {
#include <libnotify/notify.h>
}
#define signals public
#endif

QTrayIcon::QTrayIcon(QWidget *obj_parent)
    : TrayIndicator(obj_parent)
{
#ifdef Q_OS_LINUX
    notify_init("Qcma");
#endif
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

QTrayIcon::~QTrayIcon()
{
#ifdef Q_OS_LINUX
    notify_uninit();
#endif
}

void QTrayIcon::init()
{
    options = new QAction(tr("Settings"), this);
    reload = new QAction(tr("Refresh database"), this);
    backup = new QAction(tr("Backup Manager"), this);
    about = new QAction(tr("About Qcma"), this);
    about_qt = new QAction(tr("About Qt"), this);
    quit = new QAction(tr("Quit"), this);

    connect(options, SIGNAL(triggered()), this, SIGNAL(openConfig()));
    connect(backup, SIGNAL(triggered()), this, SIGNAL(openManager()));
    connect(reload, SIGNAL(triggered()), this, SIGNAL(refreshDatabase()));
    connect(about, SIGNAL(triggered()), this, SIGNAL(showAboutDialog()));
    connect(about_qt, SIGNAL(triggered()), this, SIGNAL(showAboutQt()));
    connect(quit, SIGNAL(triggered()), this, SIGNAL(stopServer()));

    QMenu *tray_icon_menu = new QMenu(this);

    tray_icon_menu->addAction(options);
    tray_icon_menu->addAction(reload);
    tray_icon_menu->addAction(backup);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(about);
    tray_icon_menu->addAction(about_qt);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(quit);

    m_tray_icon = new QSystemTrayIcon(this);
    m_tray_icon->setToolTip("Qcma");
    m_tray_icon->setContextMenu(tray_icon_menu);
}


void QTrayIcon::showMessage(const QString &title, const QString &message)
{
#ifdef Q_OS_LINUX
    NotifyNotification *notif = notify_notification_new(qPrintable(title), qPrintable(message), "dialog-information");
    notify_notification_show(notif, NULL);
    g_object_unref(G_OBJECT(notif));
#else
    m_tray_icon->showMessage(title, message);
#endif
}

bool QTrayIcon::isVisible()
{
    return m_tray_icon->isVisible();
}

void QTrayIcon::setIcon(const QString &icon)
{
    QIcon qicon(":/main/resources/images/" + icon + ".png");
    m_tray_icon->setIcon(qicon);
}

void QTrayIcon::show()
{
    m_tray_icon->show();
}

void QTrayIcon::hide()
{
    m_tray_icon->hide();
}

// exported library function
TrayIndicator *createTrayIndicator(QWidget *parent)
{
    return new QTrayIcon(parent);
}
