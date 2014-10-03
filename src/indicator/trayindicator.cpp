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

#include "trayindicator.h"

#undef signals
extern "C" {
#include <libnotify/notify.h>
}
#define signals public


TrayIndicator::TrayIndicator(QWidget *parent)
    : QWidget(parent)
{
#ifdef Q_OS_LINUX
    notify_init("qcma");
#endif
}

TrayIndicator::~TrayIndicator()
{
#ifdef Q_OS_LINUX
    notify_uninit();
#endif
}

void TrayIndicator::showMessage(const QString &title, const QString &message)
{
#ifdef Q_OS_LINUX
    NotifyNotification *notif = notify_notification_new(qPrintable(title), qPrintable(message), "dialog-information");
    notify_notification_show(notif, NULL);
    g_object_unref(G_OBJECT(notif));
#else
    Q_UNUSED(title);
    Q_UNUSED(message);
#endif
}
