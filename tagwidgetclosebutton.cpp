#include "tagwidgetclosebutton.h"

TagWidgetCloseButton::TagWidgetCloseButton(QWidget *parent)
    : QPushButton(parent) {

    TaggingWidget *p = qobject_cast<TaggingWidget*>(parentWidget());
    if (p) {
        background_color_ = p->base_color_;
    } else {
        background_color_ = QColor::fromHsv(0,200,200); // Default color if this item has an incorrect parent
    }

    int bright_value = background_color_.value() + 15;
    brighter_edge_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), bright_value);

    int dark_value = background_color_.value() - 25;
    darker_edge_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), dark_value);
}

void TagWidgetCloseButton::paintEvent(QPaintEvent *event){

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int circleInset = 6;
    int circleWidth = 28;
    int circleRightBottom = circleWidth - (circleInset * 2);
    int circleRadius = 8;
    double xInset = 11.5;
    double xRightBottom = xInset + 5;

    // Paint circular button
    painter.setBrush(background_color_);
    painter.setPen(Qt::NoPen);
    QRect circleBounds = QRect(circleInset, circleInset, circleRightBottom, circleRightBottom);
    painter.drawRoundedRect(circleBounds, circleRadius, circleRadius);

    QPen circlePen(darker_edge_color_);
    circlePen.setWidth(2);
    circlePen.setCapStyle(Qt::FlatCap);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(circlePen);
    painter.drawArc(circleBounds, 45 * 16, 180 * 16);

    circlePen.setColor(brighter_edge_color_);
    painter.setPen(circlePen);
    painter.drawArc(circleBounds, 225 * 16, 180 * 16);

    // Paint 'x'
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
