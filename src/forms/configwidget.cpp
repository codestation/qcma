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
    setDefaultData();
}

void ConfigWidget::connectSignals()
{
    QSignalMapper *mapper = new QSignalMapper(this);
    mapper->setMapping(ui->photoBtn, BTN_PHOTO);
    mapper->setMapping(ui->musicBtn, BTN_MUSIC);
    mapper->setMapping(ui->videoBtn, BTN_VIDEO);
    mapper->setMapping(ui->appBtn, BTN_APPS);
    mapper->setMapping(ui->urlBtn, BTN_URL);
    mapper->setMapping(ui->pkgBtn, BTN_PKG);
    connect(ui->photoBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->musicBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->videoBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->appBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->urlBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->pkgBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(browseBtnPressed(int)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ConfigWidget::setDefaultData()
{
    QString defaultdir;
    QSettings settings;
    defaultdir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    ui->photoPath->setText(QDir::toNativeSeparators(settings.value("photoPath", defaultdir).toString()));

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    ui->musicPath->setText(QDir::toNativeSeparators(settings.value("musicPath", defaultdir).toString()));

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    ui->videoPath->setText(QDir::toNativeSeparators(settings.value("videoPath", defaultdir).toString()));

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PS Vita");
    ui->appPath->setText(QDir::toNativeSeparators(settings.value("appsPath", defaultdir).toString()));

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PSV Updates");
    ui->urlPath->setText(QDir::toNativeSeparators(settings.value("urlPath", defaultdir).toString()));

    defaultdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    defaultdir.append(QDir::separator()).append("PSV Packages");
    ui->pkgPath->setText(QDir::toNativeSeparators(settings.value("pkgPath", defaultdir).toString()));

    ui->offlineCheck->setChecked(settings.value("offlineMode", true).toBool());
    ui->metadataCheck->setChecked(settings.value("skipMetadata", false).toBool());
    ui->usbCheck->setChecked(settings.value("disableUSB", false).toBool());
    ui->wifiCheck->setChecked(settings.value("disableWireless", false).toBool());
    ui->databaseSelect->setCurrentIndex(settings.value("useMemoryStorage", true).toBool() ? 0 : 1);

    ui->photoSkipCheck->setChecked(settings.value("photoSkip", false).toBool());
    ui->videoSkipCheck->setChecked(settings.value("videoSkip", false).toBool());
    ui->musicSkipCheck->setChecked(settings.value("musicSkip", false).toBool());
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

    case BTN_PKG:
        lineedit = ui->pkgPath;
        msg = tr("Select the folder to be used to software packages");
        break;

    default:
        return;
    }

    QString selected = QFileDialog::getExistingDirectory(this, msg, lineedit->text(), QFileDialog::ShowDirsOnly);

    if(!selected.isEmpty()) {
        lineedit->setText(QDir::toNativeSeparators((selected)));
    }
}

void ConfigWidget::savePath(QSettings &settings, const QLineEdit *edit, const QString &key)
{
    QString path = edit->text();
    if(path.endsWith(QDir::separator())) {
        path.chop(1);
    }
    settings.setValue(key, QDir::fromNativeSeparators(path));
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
    savePath(settings, ui->pkgPath, "pkgPath");
    settings.setValue("offlineMode", ui->offlineCheck->isChecked());
    settings.setValue("skipMetadata", ui->metadataCheck->isChecked());
    settings.setValue("disableUSB", ui->usbCheck->isChecked());
    settings.setValue("disableWireless", ui->wifiCheck->isChecked());
    settings.setValue("useMemoryStorage", ui->databaseSelect->currentIndex() == 0);
    settings.setValue("photoSkip", ui->photoSkipCheck->isChecked());
    settings.setValue("videoSkip", ui->videoSkipCheck->isChecked());
    settings.setValue("musicSkip", ui->musicSkipCheck->isChecked());
    settings.sync();

    done(Accepted);
}
