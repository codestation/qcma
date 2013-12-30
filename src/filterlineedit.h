/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2013  Xian Nox
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

#ifndef FILTERLINEEDIT_H
#define FILTERLINEEDIT_H

#include <QLineEdit>
#include <QToolButton>

class FilterLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit FilterLineEdit(QWidget *parent = 0);

protected:
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void resizeEvent(QResizeEvent *e);
    int frameWidth() const;

private:
    QToolButton *clearButton;

private slots:
    void updateCloseButton(const QString &text);

};

#endif // FILTERLINEEDIT_H
