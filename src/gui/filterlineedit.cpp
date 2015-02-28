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

FilterLineEdit::FilterLineEdit(QWidget *obj_parent) :
    QLineEdit(obj_parent)
{
    int frame_width = frameWidth();
    clearButton = new QToolButton(this);
    QIcon clearIcon(":/main/resources/images/edit-clear-locationbar-rtl.png");
    clearButton->setIcon(clearIcon);
    clearButton->setIconSize(QSize(sizeHint().height() - 4 * frame_width,
                                   sizeHint().height() - 4 * frame_width));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("QToolButton { border:none; padding:0px; }");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));

    setStyleSheet(QString("LineEdit { color:black; font-style:normal; padding-right:%1px; }").arg(
                      clearButton->sizeHint().width() + frame_width + 1));

    QSize min_size_hint = minimumSizeHint();
    setMinimumSize(qMax(min_size_hint.width(), clearButton->sizeHint().height() + frame_width),
                   qMax(min_size_hint.height(), clearButton->sizeHint().height() + frame_width));
}

void FilterLineEdit::updateCloseButton(const QString& filter_text)
{
    if(filter_text.isEmpty() || filter_text == tr("Filter")) {
        clearButton->setVisible(false);
    } else {
        clearButton->setVisible(true);
    }
}

void FilterLineEdit::focusInEvent(QFocusEvent *e)
{
    if(this->styleSheet() == "FilterLineEdit { color:gray; font-style:italic; }") {
        this->clear();
    }

    setStyleSheet(QString("FilterLineEdit { color:black; font-style:normal; padding-right:%1px; }").arg(clearButton->sizeHint().width() + frameWidth() + 1));

    QLineEdit::focusInEvent(e);
}

void FilterLineEdit::focusOutEvent(QFocusEvent *e)
{
    if(this->text().isEmpty()) {
        this->setText(tr("Filter"));
        this->setStyleSheet("FilterLineEdit { color:gray; font-style:italic; }");
    }

    QLineEdit::focusOutEvent(e);
}

int FilterLineEdit::frameWidth() const
{
    return style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
}

void FilterLineEdit::resizeEvent(QResizeEvent *e)
{
    QSize sz = clearButton->sizeHint();
    clearButton->move(rect().right() - sz.width(), rect().bottom() - sz.height() + frameWidth());

    QLineEdit::resizeEvent(e);
}
