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

#ifndef PINFORM_H
#define PINFORM_H

#include <QTimer>
#include <QWidget>

namespace Ui {
class PinForm;
}

class PinForm : public QWidget
{
    Q_OBJECT

public:
    explicit PinForm(QWidget *parent = 0);
    ~PinForm();

private:
    Ui::PinForm *ui;

    // pin timeout
    int counter;
    QTimer timer;

    static const QString pinFormat;

public slots:
    void startCountdown();
    void setPin(QString name, int pin);

private slots:
    void decreaseTimer();
};

#endif // PINFORM_H
