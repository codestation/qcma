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

#include "kdenotifiertray.h"
#ifndef ENABLE_KNOTIFICATIONS
#include <kmenu.h>
#else
#include <QMenu>
#endif

KDENotifierTray::KDENotifierTray(QWidget *obj_parent)
    : TrayIndicator(obj_parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void KDENotifierTray::init()
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

#ifndef ENABLE_KNOTIFICATIONS
    KMenu *tray_icon_menu = new KMenu(this);
#else
    QMenu *tray_icon_menu = new QMenu(this);
#endif

    tray_icon_menu->addAction(options);
    tray_icon_menu->addAction(reload);
    tray_icon_menu->addAction(backup);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(about);
    tray_icon_menu->addAction(about_qt);
    tray_icon_menu->addSeparator();
    tray_icon_menu->addAction(quit);

    m_notifier_item = new KDENotifier("QcmaNotifier", this);
    m_notifier_item->setContextMenu(tray_icon_menu);
    m_notifier_item->setTitle("Qcma");
    m_notifier_item->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_notifier_item->setIconByPixmap(QIcon(":/main/resources/images/qcma_off.png"));
    m_notifier_item->setStatus(KStatusNotifierItem::Active);
    m_notifier_item->setToolTipTitle(tr("Qcma status"));
    m_notifier_item->setToolTipIconByPixmap(QIcon(":/main/resources/images/qcma.png"));
    m_notifier_item->setToolTipSubTitle(tr("Disconnected"));
    m_notifier_item->setStandardActionsEnabled(false);
}

void KDENotifierTray::showMessage(const QString &title, const QString &message)
{
    m_notifier_item->showMessage(title, message, "dialog-information", 3000);
}

bool KDENotifierTray::isVisible()
{
    return true;
}

void KDENotifierTray::setIcon(const QString &icon)
{
    m_notifier_item->setIconByPixmap(QIcon(":/main/resources/images/" + icon));
}

void KDENotifierTray::show()
{
}

void KDENotifierTray::hide()
{
}

// exported library function
TrayIndicator *createTrayIndicator(QWidget *parent)
{
    return new KDENotifierTray(parent);
}
