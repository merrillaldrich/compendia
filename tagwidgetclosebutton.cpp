#include "tagwidgetclosebutton.h"

TagWidgetCloseButton::TagWidgetCloseButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent) {


}

void TagWidgetCloseButton::paintEvent(QPaintEvent *event){

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int circleInset = 6;
    int circleWidth = 28;
    int circleRightBottom = circleWidth - (circleInset * 2);
    int circleRadius = 8;
    int xInset = 12; // circleInset + 4;
    int xRightBottom = xInset + 5; //circleRightBottom - 4;

    QColor bgColor = "#FFFFFF";
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(circleInset, circleInset, circleRightBottom, circleRightBottom, circleRadius, circleRadius);

    QPainterPath path;

    QPen xPen(QColor("#888888"));
    xPen.setWidth(2);
    painter.setPen(xPen);

    path.moveTo(xInset, xInset);
    path.lineTo(xRightBottom, xRightBottom);
    path.moveTo(xInset, xRightBottom);
    path.lineTo(xRightBottom, xInset);
    painter.drawPath(path);
}
