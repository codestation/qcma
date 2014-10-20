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

#ifndef KDENOTIFIER_H
#define KDENOTIFIER_H

#include <kstatusnotifieritem.h>
#if QT_VERSION < 0x050000
#include <kmenu.h>
#endif

class KDENotifier : public KStatusNotifierItem
{
    Q_OBJECT
public:
    explicit KDENotifier(const QString &id, QObject *parent = 0);

signals:

public slots:
    // block left click because it shows the default widget
    virtual void activate (const QPoint &pos=QPoint()) {
        Q_UNUSED(pos);
    }
};

#endif // KDENOTIFIER_H
