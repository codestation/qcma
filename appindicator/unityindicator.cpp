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

#include "unityindicator.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#define QStandardPaths QDesktopServices
#define writableLocation storageLocation
#endif

#include <QDebug>
#include <QDir>
#include <QVector>

#undef signals

extern "C" {

#include <libnotify/notify.h>

void optionsIndicator(GtkMenu *menu, gpointer data);
void reloadIndicator(GtkMenu *menu, gpointer data);
void backupIndicator(GtkMenu *menu, gpointer data);
void aboutIndicator(GtkMenu *menu, gpointer data);
void aboutQtIndicator(GtkMenu *menu, gpointer data);
void quitIndicator(GtkMenu *menu, gpointer data);
}

#define signals public

UnityIndicator::UnityIndicator(QWidget *obj_parent) :
    TrayIndicator(obj_parent)
{
    notify_init("qcma");
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

UnityIndicator::~UnityIndicator()
{
    notify_uninit();
    for(QVector<QPair<gpointer, gulong> >::iterator it = m_handlers.begin(); it != m_handlers.end(); ++it)
    {
        g_signal_handler_disconnect(it->first, it->second);
    }
}

void optionsIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->openConfig();
}

void reloadIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->refreshDatabase();
}

void backupIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->openManager();
}

void aboutIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->showAboutDialog();
}

void aboutQtIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->showAboutQt();
}

void quitIndicator(GtkMenu *, gpointer data)
{
    UnityIndicator *m_this = reinterpret_cast<UnityIndicator *>(data);
    emit m_this->stopServer();
}

void UnityIndicator::init()
{
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *options = gtk_menu_item_new_with_label(qPrintable(tr("Settings")));
    GtkWidget *reload = gtk_menu_item_new_with_label(qPrintable(tr("Refresh database")));
    GtkWidget *backup = gtk_menu_item_new_with_label(qPrintable(tr("Backup Manager")));
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *about = gtk_menu_item_new_with_label(qPrintable(tr("About QCMA")));
    GtkWidget *about_qt = gtk_menu_item_new_with_label(qPrintable(tr("About Qt")));
    GtkWidget *separator2 = gtk_separator_menu_item_new();
    GtkWidget *quit = gtk_menu_item_new_with_label(qPrintable(tr("Quit")));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), options);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), reload);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), backup);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), about);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), about_qt);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);

    gulong gobject_handler;
    gobject_handler = g_signal_connect(options, "activate", G_CALLBACK(optionsIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(options, gobject_handler));
    gobject_handler = g_signal_connect(reload, "activate", G_CALLBACK(reloadIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(reload, gobject_handler));
    gobject_handler = g_signal_connect(backup, "activate", G_CALLBACK(backupIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(backup, gobject_handler));
    gobject_handler = g_signal_connect(about, "activate", G_CALLBACK(aboutIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(about, gobject_handler));
    gobject_handler = g_signal_connect(about_qt, "activate", G_CALLBACK(aboutQtIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(about_qt, gobject_handler));
    gobject_handler = g_signal_connect(quit, "activate", G_CALLBACK(quitIndicator), this);
    m_handlers.append(QPair<gpointer, gulong>(quit, gobject_handler));

    gtk_widget_show(options);
    gtk_widget_show(reload);
    gtk_widget_show(backup);
    gtk_widget_show(separator1);
    gtk_widget_show(about);
    gtk_widget_show(about_qt);
    gtk_widget_show(separator2);
    gtk_widget_show(quit);

    m_indicator = app_indicator_new(
        "qcma-appindicator",
        "qcma-messages",
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    );

    QString icon_path;
    QString icon_name = "share/icons/hicolor/64x64/actions/qcma_on.png";

    if(QFile("/usr/" + icon_name).exists())
        icon_path = QFileInfo("/usr/" + icon_name).absolutePath();
    else if(QFile("/usr/local" + icon_name).exists())
        icon_path = QFileInfo("/usr/" + icon_name).absolutePath();

    if(!icon_path.isEmpty())
    {
        qDebug() << "Using " << icon_path << " as icon theme path";
        app_indicator_set_icon_theme_path(m_indicator, qPrintable(icon_path));
    }
    else
        qWarning() << "Cannot find qcma icons.";

    app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu(m_indicator, GTK_MENU(menu));
}

bool UnityIndicator::isVisible()
{
    return true;
}

void UnityIndicator::setIcon(const QString &icon)
{
    app_indicator_set_icon_full(m_indicator, qPrintable(icon), "icon");
}

void UnityIndicator::show()
{
}

void UnityIndicator::hide()
{
}

void UnityIndicator::showMessage(const QString &title, const QString &message)
{
    NotifyNotification *notif = notify_notification_new(qPrintable(title), qPrintable(message), "dialog-information");
    notify_notification_show(notif, NULL);
    g_object_unref(G_OBJECT(notif));
}

// exported library function
TrayIndicator *createTrayIndicator(QWidget *parent)
{
    return new UnityIndicator(parent);
}
