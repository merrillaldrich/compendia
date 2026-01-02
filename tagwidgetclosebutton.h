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
    QColor brighter_edge_color_;
    QColor darker_edge_color_;

protected:
    void paintEvent(QPaintEvent *event) override;

};

#endif // TAGWIDGETCLOSEBUTTON_H
