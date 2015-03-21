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

#include "confirmdialog.h"
#include "ui_confirmdialog.h"

const QString ConfirmDialog::messageTemplate = "<html><head/><body>"
        "<p><span style=\"font-size:10pt;\">%1</span></p>"
        "<p><span style=\"font-size:12pt; font-weight:600;\">%2</span></p>"
        "</body></html>";

ConfirmDialog::ConfirmDialog(QWidget *obj_parent) :
    QDialog(obj_parent),
    ui(new Ui::ConfirmDialog)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void ConfirmDialog::setMessageText(const QString message, const QString game_title)
{
    ui->confirmText->setText(messageTemplate.arg(message, game_title));
}

void ConfirmDialog::setMessagePixmap(const QPixmap &pixmap, int dialog_width)
{
    ui->itemPicture->setPixmap(pixmap);
    ui->itemPicture->setMinimumWidth(dialog_width);
}

ConfirmDialog::~ConfirmDialog()
{
    delete ui;
}
