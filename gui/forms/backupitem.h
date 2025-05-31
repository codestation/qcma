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

#ifndef BACKUPITEM_H
#define BACKUPITEM_H

#include <QWidget>

namespace Ui {
class BackupItem;
}

class BackupItem : public QWidget
{
    Q_OBJECT

public:
    explicit BackupItem(QWidget *parent = 0);
    ~BackupItem();

    void setItemInfo(const QString &name, const QString &size, const QString &extra);
    void setItemIcon(const QString &m_path, int width = 48, bool try_dds = false);
    void setDirectory(const QString &m_path);
    const QPixmap getIconPixmap();
    int getIconWidth();
    QString getPath();
    QString getSize();

    static bool lessThan(const BackupItem *s1, const BackupItem *s2);

    int ohfi;
    QString title;

private:
    QString m_path;
    Ui::BackupItem *ui;
    static const QString gameTemplate;
    static const QString sizeTemplate;
    static const QString infoTemplate;

signals:
    void deleteEntry(BackupItem *entry);

private slots:
    void openDirectory();
    void removeEntry();
};

#endif // BACKUPITEM_H
