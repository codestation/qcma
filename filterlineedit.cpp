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

#include "filterlineedit.h"

#include <QIcon>
#include <QStyle>

FilterLineEdit::FilterLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    QIcon clearIcon(":/main/images/edit-clear-locationbar-rtl.png");
    clearButton->setIcon(clearIcon);
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("border:none;padding:0px");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));
}

void FilterLineEdit::updateCloseButton(const QString& text)
{
    if(text.isEmpty() || text == tr("Filter")) {
        clearButton->setVisible(false);
    } else {
        clearButton->setVisible(true);
    }
}

void FilterLineEdit::focusInEvent(QFocusEvent *e)
{
    if(this->styleSheet() == "color:gray;font-style:italic") {
        this->clear();
    }

    this->setStyleSheet(QString("color:black;font-style:normal;padding-right:%1").arg(clearButton->sizeHint().width()));

    QLineEdit::focusInEvent(e);
}

void FilterLineEdit::focusOutEvent(QFocusEvent *e)
{
    if(this->text().isEmpty()) {
        this->setText(tr("Filter"));
        this->setStyleSheet("color:gray;font-style:italic");
    }

    QLineEdit::focusOutEvent(e);
}

void FilterLineEdit::resizeEvent(QResizeEvent *e)
{
    QSize sz = clearButton->sizeHint();
    clearButton->move(rect().right() - sz.width(), (rect().bottom() - sz.height()) / 2);

    QLineEdit::resizeEvent(e);
}
