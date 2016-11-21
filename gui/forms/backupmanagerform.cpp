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

#include "backupmanagerform.h"
#include "ui_backupmanagerform.h"
#include "cmaobject.h"
#include "sforeader.h"
#include "confirmdialog.h"
#include "cmautils.h"
#include "filterlineedit.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QJsonDocument>
#include <QSettings>

#include <vitamtp.h>

BackupManagerForm::BackupManagerForm(Database *db, QWidget *obj_parent) :
    QDialog(obj_parent), m_db(db),
    ui(new Ui::BackupManagerForm)
{
    ui->setupUi(this);
    setupForm();
}

BackupManagerForm::~BackupManagerForm()
{
    delete ui;
}

void BackupManagerForm::setupForm()
{
    this->resize(800, 480);
    connect(ui->backupComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadBackupListing(int)));
    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveListing()));
    ui->tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->horizontalHeader()->hide();
}

void BackupManagerForm::saveListing()
{
    int rows = ui->tableWidget->rowCount();
    QVariantList items;

    for(int i = 0; i < rows; ++i) {
        BackupItem *item = static_cast<BackupItem *>(ui->tableWidget->cellWidget(i, 0));

        QVariantMap entry;
        entry.insert("title", item->title);
        entry.insert("gameid", QFileInfo(item->getPath()).fileName());
        entry.insert("path", item->getPath());
        entry.insert("size", item->getSize());

        items.append(entry);
    }

    QString output = QFileDialog::getSaveFileName(
            this, tr("Select save location"), QDir::homePath() + "/listing.json",
                "JSON (*.json)");

    if(!output.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromVariant(items);
        QFile jsonFile(output);
        jsonFile.open(QFile::WriteOnly);
        jsonFile.write(doc.toJson());
    }
}

void BackupManagerForm::removeEntry(BackupItem *item)
{
    ConfirmDialog msgBox;

    msgBox.setMessageText(tr("Are you sure to remove the backup of the following entry?"), item->title);
    msgBox.setMessagePixmap(*item->getIconPixmap(), item->getIconWidth());

    if(msgBox.exec() == 0) {
        return;
    }

    QMutexLocker locker(&m_db->mutex);

    int parent_ohfi = m_db->getParentId(item->ohfi);
    removeRecursively(m_db->getAbsolutePath(item->ohfi));
    m_db->deleteEntry(item->ohfi);

    for(int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        BackupItem *iter_item = static_cast<BackupItem *>(ui->tableWidget->cellWidget(i, 0));
        if(iter_item == item) {
            ui->tableWidget->removeRow(i);
            break;
        }
    }

    if(parent_ohfi > 0) {
        setBackupUsage(m_db->getObjectSize(parent_ohfi));
    }
}

void BackupManagerForm::setBackupUsage(qint64 usage_size)
{
    ui->usageLabel->setText(tr("Backup disk usage: %1").arg(readable_size(usage_size, true)));
}

void BackupManagerForm::loadBackupListing(int index)
{
    int ohfi;
    bool sys_dir;
    int img_width;

    //TODO: load all the accounts in the combobox
    ui->accountBox->clear();
    ui->accountBox->addItem(QSettings().value("lastOnlineId", tr("Default account")).toString());

    if(index < 0) {
        index = ui->backupComboBox->currentIndex();
    }

    ui->tableWidget->clear();

    switch(index) {
    case 0:
        ohfi = VITA_OHFI_VITAAPP;
        img_width = 48;
        sys_dir = true;
        break;
    case 1:
        ohfi = VITA_OHFI_PSPAPP;
        img_width = 80;
        sys_dir = true;
        break;
    case 2:
        ohfi = VITA_OHFI_PSMAPP;
        img_width = 48;
        sys_dir = true;
        break;
    case 3:
        ohfi = VITA_OHFI_PSXAPP;
        img_width = 48;
        sys_dir = true;
        break;
    case 4:
        ohfi = VITA_OHFI_PSPSAVE;
        img_width = 80;
        sys_dir = false;
        break;
    case 5:
        ohfi = VITA_OHFI_BACKUP;
        img_width = 48;
        sys_dir = false;
        break;
    default:
        ohfi = VITA_OHFI_VITAAPP;
        img_width = 48;
        sys_dir = true;
    }

    m_db->mutex.lock();

    // get the item list
    metadata_t *meta = NULL;
    int row_count = m_db->getObjectMetadatas(ohfi, &meta);
    ui->tableWidget->setRowCount(row_count);

    // exit if there aren't any items
    if(row_count == 0) {
        setBackupUsage(0);
        m_db->mutex.unlock();
        return;
    }

    // adjust the table item width to fill all the widget
    QHeaderView *vert_header = ui->tableWidget->verticalHeader();
    QHeaderView *horiz_header = ui->tableWidget->horizontalHeader();
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    horiz_header->setSectionResizeMode(QHeaderView::Stretch);
#else
    horiz_header->setResizeMode(QHeaderView::Stretch);
#endif
    qint64 backup_size = m_db->getObjectSize(ohfi);
    setBackupUsage(backup_size);
    QString path = m_db->getAbsolutePath(ohfi);
    QList<BackupItem *> item_list;

    metadata_t *first = meta;
    while(meta) {
        QString base_path = path + QDir::separator() + meta->name;
        QString parent_path = sys_dir ? base_path + QDir::separator() + "sce_sys" : base_path;
        SfoReader reader;
        QString game_name;

        // retrieve the game name from the SFO
        if(reader.load(QDir(parent_path).absoluteFilePath(sys_dir ? "param.sfo" : "PARAM.SFO"))) {
            game_name = QString::fromUtf8(reader.value("TITLE", meta->name));
        } else {
            game_name = QString(meta->name);
        }

        BackupItem *item = new BackupItem();
        // save the game title and ohfi for sorting/deleting
        item->ohfi = meta->ohfi;
        item->title = game_name;

        connect(item, SIGNAL(deleteEntry(BackupItem*)), this, SLOT(removeEntry(BackupItem*)));

        // show better size info for multi GiB backups
        bool use_gb = ohfi == VITA_OHFI_BACKUP && meta->size > 1024*1024*1024;
        QString read_size = readable_size(meta->size, use_gb);

        QString info;

        // check if is listing PS Vita games
        if(index == 0) {
            if(QDir(base_path + QDir::separator() + "app").exists()) {
                info.append(tr(" [GAME]"));
            }
            if(QDir(base_path + QDir::separator() + "savedata").exists()) {
                info.append(tr(" [SAVE]"));
            }
            if(QDir(base_path + QDir::separator() + "patch").exists()) {
                info.append(tr(" [UPDATE]"));
            }
            if(QDir(base_path + QDir::separator() + "addcont").exists()) {
                info.append(tr(" [DLC]"));
            }
        }

        item->setItemInfo(game_name, read_size, info);
        item->setItemIcon(QDir(parent_path).absoluteFilePath(sys_dir ? "icon0.png" : "ICON0.PNG"), img_width, ohfi == VITA_OHFI_PSMAPP);
        item->setDirectory(path + QDir::separator() + meta->name);
        item->resize(646, 68);

        item_list << item;
        meta = meta->next_metadata;
    }

    m_db->freeMetadata(first);

    qSort(item_list.begin(), item_list.end(), BackupItem::lessThan);

    int row;
    QList<BackupItem *>::iterator it;
    vert_header->setUpdatesEnabled(false);

    // insert the sorted items into the table
    for(it = item_list.begin(), row = 0; it != item_list.end(); ++it, ++row) {
        ui->tableWidget->setCellWidget(row, 0, *it);
        vert_header->resizeSection(row, 68);
    }

    vert_header->setUpdatesEnabled(true);
    m_db->mutex.unlock();

    // apply filter
    this->on_filterLineEdit_textChanged(ui->filterLineEdit->text());
}

void BackupManagerForm::on_filterLineEdit_textChanged(const QString &arg1)
{
    if(arg1 != tr("Filter")) {
        for(int i = 0; i < ui->tableWidget->rowCount(); ++i) {
            BackupItem *item = (BackupItem*) ui->tableWidget->cellWidget(i, 0);

            if(item->title.contains(arg1, Qt::CaseInsensitive)) {
                ui->tableWidget->setRowHidden(i, false);
            } else {
                ui->tableWidget->setRowHidden(i, true);
            }
        }
    }
}
