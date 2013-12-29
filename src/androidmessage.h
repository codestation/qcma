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

#ifndef ANDROIDMESSAGE_H
#define ANDROIDMESSAGE_H

#include <QObject>

class AndroidMessage : public QObject
{
    Q_OBJECT
public:
    explicit AndroidMessage(QObject *parent = 0);

public slots:
    void showToast(const QString &message);
    void showDialog(const QString &message);
};

#endif // ANDROIDMESSAGE_H
