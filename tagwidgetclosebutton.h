#ifndef TAGWIDGETCLOSEBUTTON_H
#define TAGWIDGETCLOSEBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>

class TagWidgetCloseButton : public QPushButton
{
    Q_OBJECT

public:
    explicit TagWidgetCloseButton(const QString &text, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

};

#endif // TAGWIDGETCLOSEBUTTON_H
