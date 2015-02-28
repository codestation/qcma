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

#include "progressform.h"
#include "ui_progressform.h"

#include <QDesktopWidget>
#include <QMessageBox>

ProgressForm::ProgressForm(QWidget *obj_parent) :
    QWidget(obj_parent),
    ui(new Ui::ProgressForm)
{
    ui->setupUi(this);
    move(QApplication::desktop()->screen()->rect().center() - rect().center());
    setFixedSize(size());
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancelConfirm()));
}

ProgressForm::~ProgressForm()
{
    delete ui;
}

void ProgressForm::cancelConfirm()
{
    QMessageBox box;
    box.setText(tr("Database indexing in progress"));
    box.setInformativeText(tr("Are you sure to cancel the database indexing?"));
    box.setIcon(QMessageBox::Warning);
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if(box.exec() == QMessageBox::Ok) {
        emit canceled();
    }
}

void ProgressForm::setFileName(QString file)
{
    QString elided = ui->fileLabel->fontMetrics().elidedText(file, Qt::ElideMiddle, ui->fileLabel->width(), 0);
    ui->fileLabel->setText(elided);
}

void ProgressForm::setDirectoryName(QString dir)
{
    QString elided = ui->directoryLabel->fontMetrics().elidedText(dir, Qt::ElideMiddle, ui->directoryLabel->width(), 0);
    ui->directoryLabel->setText(elided);
}

void ProgressForm::showDelayed(int msec)
{
    timer.setSingleShot(true);
    timer.setInterval(msec);
    connect(&timer, SIGNAL(timeout()), this, SLOT(show()));
    timer.start();
}

void ProgressForm::interruptShow()
{
    timer.stop();
}
