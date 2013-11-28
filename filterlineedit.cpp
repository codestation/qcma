#include "filterlineedit.h"

#include <QStyle>
#include <QIcon>

FilterLineEdit::FilterLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    QIcon clearIcon(":/main/resources/edit-clear-locationbar-rtl.png");
    clearButton->setIcon(clearIcon);
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("border:none;padding:0px");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));
}

void FilterLineEdit::updateCloseButton(const QString& text)
{
    if(text.isEmpty() || text == tr("Filter"))
        clearButton->setVisible(false);
    else
        clearButton->setVisible(true);
}

void FilterLineEdit::focusInEvent(QFocusEvent *e)
{
    if(this->styleSheet() == "color:gray;font-style:italic")
        this->clear();

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
    clearButton->move(rect().right() - sz.width(), (rect().bottom() - sz.height())/2);

    QLineEdit::resizeEvent(e);
}
