#ifndef TAGWIDGETCLOSEBUTTON_H
#define TAGWIDGETCLOSEBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include "taggingwidget.h"

class TagWidgetCloseButton : public QPushButton
{
    Q_OBJECT

public:
    explicit TagWidgetCloseButton(QWidget *parent = nullptr);

private:
    QColor background_color_;
    QColor highlight_edge_color_;
    QColor shadow_edge_color_;
    QColor mouse_over_color_;
    QColor effective_color_;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

#endif // TAGWIDGETCLOSEBUTTON_H
