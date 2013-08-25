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

#include "configwidget.h"
#include "ui_configwidget.h"

extern "C" {
#include <vitamtp.h>
}

#include <QFileDialog>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#define QStandardPaths QDesktopServices
#define writableLocation storageLocation
#endif

ConfigWidget::ConfigWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigWidget)
{
    ui->setupUi(this);
    connectSignals();
    setDefaultDirs();
}

void ConfigWidget::connectSignals()
{
    QSignalMapper *mapper = new QSignalMapper(this);
    mapper->setMapping(ui->photoBtn, BTN_PHOTO);
    mapper->setMapping(ui->musicBtn, BTN_MUSIC);
    mapper->setMapping(ui->videoBtn, BTN_VIDEO);
    mapper->setMapping(ui->appBtn, BTN_APPS);
    mapper->setMapping(ui->urlBtn, BTN_URL);
    connect(ui->photoBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->musicBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->videoBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->appBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->urlBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(browseBtnPressed(int)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ConfigWidget::setDefaultDirs()
{
    QString defaultdir;
    QSettings settings;
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    ui->photoPath->setText(settings.value("photoPath", defaultdir).toString());
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    ui->musicPath->setText(settings.value("musicPath", defaultdir).toString());
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    ui->videoPath->setText(settings.value("videoPath", defaultdir).toString());
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PS Vita");
    ui->appPath->setText(settings.value("appsPath", defaultdir).toString());
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PSV Updates");
    ui->urlPath->setText(settings.value("urlPath", defaultdir).toString());
}

ConfigWidget::~ConfigWidget()
{
    delete ui;
}

void ConfigWidget::browseBtnPressed(int btn)
{
    QString msg;
    QLineEdit *lineedit;

    switch(btn) {
    case BTN_PHOTO:
        lineedit = ui->photoPath;
        msg = tr("Select the folder to be used as a photo source");
        break;

    case BTN_MUSIC:
        lineedit = ui->musicPath;
        msg = tr("Select the folder to be used as a music source");
        break;

    case BTN_VIDEO:
        lineedit = ui->videoPath;
        msg = tr("Select the folder to be used as a video source");
        break;

    case BTN_APPS:
        lineedit = ui->appPath;
        msg = tr("Select the folder to be used to save PS Vita games and backups");
        break;

    case BTN_URL:
        lineedit = ui->urlPath;
        msg = tr("Select the folder to be used to fetch software updates");
        break;

    default:
        return;
    }

    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    dialog.setDirectory(lineedit->text());
    dialog.setWindowTitle(msg);

    if(dialog.exec()) {
        QStringList list = dialog.selectedFiles();
        lineedit->setText(list.first());
    }
}

void ConfigWidget::savePath(QSettings &settings, const QLineEdit *edit, const QString &key)
{
    QString path = edit->text();
    if(path.endsWith(QDir::separator())) {
        path.chop(1);
    }
    settings.setValue(key, path);
    QDir(QDir::root()).mkpath(path);
}

void ConfigWidget::accept()
{
    QSettings settings;
    savePath(settings, ui->photoPath, "photoPath");
    savePath(settings, ui->musicPath, "musicPath");
    savePath(settings, ui->videoPath, "videoPath");
    savePath(settings, ui->appPath, "appsPath");
    savePath(settings, ui->urlPath, "urlPath");
    settings.sync();
    done(Accepted);
}
