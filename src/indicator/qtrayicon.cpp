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

#include <QSystemTrayIcon>

#ifdef ENABLE_KDE_NOTIFIER
#include <kmenu.h>
#else
#include <QIcon>
#include <QMenu>
#endif

QTrayIcon::QTrayIcon(QWidget *parent)
    : TrayIndicator(parent)
{
}

void QTrayIcon::init()
{
    options = new QAction(tr("Settings"), this);
    reload = new QAction(tr("Refresh database"), this);
    backup = new QAction(tr("Backup Manager"), this);
    about = new QAction(tr("About QCMA"), this);
    about_qt = new QAction(tr("About Qt"), this);
    quit = new QAction(tr("Quit"), this);

    connect(options, SIGNAL(triggered()), this, SIGNAL(openConfig()));
    connect(backup, SIGNAL(triggered()), this, SIGNAL(openManager()));
    connect(reload, SIGNAL(triggered()), this, SIGNAL(refreshDatabase()));
    connect(about, SIGNAL(triggered()), this, SIGNAL(showAboutDialog()));
    connect(about_qt, SIGNAL(triggered()), this, SIGNAL(showAboutQt()));
    connect(quit, SIGNAL(triggered()), this, SIGNAL(stopServer()));

#ifndef ENABLE_KDE_NOTIFIER
    QMenu *tray_icon_menu = new QMenu(this);
#else
    KMenu *tray_icon_menu = new KMenu(this);
#endif

    tray_icon_menu->addAction(options);
    tray_icon_menu->addAction(reload);
    tray_icon_menu->addAction(backup);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(about);
    tray_icon_menu->addAction(about_qt);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(quit);

#ifndef ENABLE_KDE_NOTIFIER
    m_tray_icon = new QSystemTrayIcon(this);
    m_tray_icon->setContextMenu(tray_icon_menu);
#else
    m_notifier_item = new KDENotifier("QcmaNotifier", this);
    m_notifier_item->setContextMenu(tray_icon_menu);
    m_notifier_item->setTitle("Qcma");
    m_notifier_item->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_notifier_item->setIconByPixmap(QIcon(":/main/resources/images/tray/qcma_off.png"));
    m_notifier_item->setStatus(KStatusNotifierItem::Active);
    m_notifier_item->setToolTipTitle(tr("Qcma status"));
    m_notifier_item->setToolTipIconByPixmap(QIcon(":/main/resources/images/qcma.png"));
    m_notifier_item->setToolTipSubTitle(tr("Disconnected"));
    m_notifier_item->setStandardActionsEnabled(false);
#endif
}

#ifndef Q_OS_LINUX
void QTrayIcon::showMessage(const QString &title, const QString &message)
{
    m_tray_icon->showMessage(title, message);
}
#endif

bool QTrayIcon::isVisible()
{
#ifndef ENABLE_KDE_NOTIFIER
    return m_tray_icon->isVisible();
#else
    return true;
#endif
}

void QTrayIcon::setIcon(const QString &icon)
{
    m_tray_icon->setIcon(QIcon(":/main/resources/images/tray/" + icon));
}

void QTrayIcon::show()
{
#ifndef ENABLE_KDE_NOTIFIER
    m_tray_icon->show();
#endif
}

void QTrayIcon::hide()
{
#ifndef ENABLE_KDE_NOTIFIER
    m_tray_icon->hide();
#endif
}
