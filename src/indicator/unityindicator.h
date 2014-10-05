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

#ifndef UNITYINDICATOR_H
#define UNITYINDICATOR_H

#include "trayindicator.h"

#include <QVector>

#undef signals

extern "C" {
#include <libappindicator/app-indicator.h>
#include <gtk/gtk.h>

void optionsIndicator(GtkMenu *menu, gpointer data);
void reloadIndicator(GtkMenu *menu, gpointer data);
void backupIndicator(GtkMenu *menu, gpointer data);
void aboutIndicator(GtkMenu *menu, gpointer data);
void aboutQtIndicator(GtkMenu *menu, gpointer data);
void quitIndicator(GtkMenu *menu, gpointer data);
}

#define signals public

class UnityIndicator : public TrayIndicator
{
    Q_OBJECT
public:
    explicit UnityIndicator(QWidget *parent = 0);
    ~UnityIndicator();
    void init();
    void setIcon(const QString &icon);
    bool isVisible();
    void show();
    void hide();
    void showMessage(const QString &title, const QString &message);

private:
    AppIndicator *m_indicator;
    QVector<QPair<gpointer, gulong> > m_handlers;
};

#endif // UNITYINDICATOR_H
