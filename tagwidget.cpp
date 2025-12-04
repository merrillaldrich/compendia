#include "tagwidget.h"

TagWidget::TagWidget(QWidget *parent)
    : QWidget{parent}
{}

TagWidget::TagWidget(Tag *tag, QWidget *parent)
    : QWidget{parent}
{
    tag_ = tag;
}

void TagWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QPen p = QPen(QBrush(QColor("orange")), 2);
    painter.setPen(p);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawRoundedRect(QRect(2, 2, 100, 24), 12, 12);

    p = QPen(QBrush(QColor("black")), 2);
    painter.setPen(p);
    painter.setFont(QFont("Segoe", 10));
    painter.drawText(QRect(22,6,100,20), Qt::AlignLeft, tag_->tagName);
}
