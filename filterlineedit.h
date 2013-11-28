#ifndef FILTERLINEEDIT_H
#define FILTERLINEEDIT_H

#include <QLineEdit>
#include <QToolButton>

class FilterLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit FilterLineEdit(QWidget *parent = 0);
    
signals:
    
public slots:

private slots:
    void updateCloseButton(const QString &text);

protected:
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);

    void resizeEvent(QResizeEvent *e);

private:
    QToolButton *clearButton;
    
};

#endif // FILTERLINEEDIT_H
