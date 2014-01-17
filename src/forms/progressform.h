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

#ifndef PROGRESSFORM_H
#define PROGRESSFORM_H

#include <QTimer>
#include <QWidget>

namespace Ui {
class ProgressForm;
}

class ProgressForm : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressForm(QWidget *parent = 0);
    ~ProgressForm();

    void showDelayed(int msec = 1000);
    void interruptShow();

private:
    Ui::ProgressForm *ui;

    QTimer timer;

signals:
    void canceled();

private slots:
    void cancelConfirm();

public slots:
    void setDirectoryName(QString dir);
    void setFileName(QString file);
};

#endif // PROGRESSFORM_H
