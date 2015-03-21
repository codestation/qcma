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

#include "pinform.h"
#include "ui_pinform.h"

#include <QDebug>
#include <QDesktopWidget>

const QString PinForm::pinFormat =
    "<html><head/><body>"
    "<p><span style=\"font-size:24pt; font-weight:600;\">%1</span></p>"
    "</body></html>";

PinForm::PinForm(QWidget *obj_parent) :
    QWidget(obj_parent),
    ui(new Ui::PinForm)
{
    ui->setupUi(this);
    move(QApplication::desktop()->screen()->rect().center() - rect().center());
    setFixedSize(size());
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(hide()));
}

void PinForm::setPin(QString name, int pin)
{
    qDebug() << "Got pin from user " << name;
    ui->deviceLabel->setText(tr("Device: %1 (PS Vita)").arg(name));
    ui->pinLabel->setText(pinFormat.arg(QString::number(pin), 8, QChar('0')));
    show();
}

void PinForm::startCountdown()
{
    timer.setInterval(1000);
    counter = 300;
    connect(&timer, SIGNAL(timeout()), this, SLOT(decreaseTimer()));
    timer.start();
}

void PinForm::decreaseTimer()
{
    counter--;
    if(counter == 0) {
        timer.stop();
        hide();
    }
    ui->timeLabel->setText(tr("Time remaining: %1 seconds").arg(counter));
}

PinForm::~PinForm()
{
    delete ui;
}
