#include "tagwidgetclosebutton.h"

TagWidgetCloseButton::TagWidgetCloseButton(QWidget *parent)
    : QPushButton(parent) {

    TaggingWidget *p = qobject_cast<TaggingWidget*>(parentWidget());
    if (p) {
        background_color_ = p->base_color_;
    } else {
        background_color_ = QColor::fromHsv(0,200,200); // Default color if this item has an incorrect parent
    }

    effective_color_ = background_color_;

    int mouse_over_value_ = background_color_.value() + 12;
    mouse_over_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), mouse_over_value_);

    int highlight_value = background_color_.value() + 15;
    highlight_edge_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), highlight_value);

    int shadow_value = background_color_.value() - 25;
    shadow_edge_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), shadow_value);
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
    painter.setBrush(effective_color_);
    painter.setPen(Qt::NoPen);
    QRect circleBounds = QRect(circleInset, circleInset, circleRightBottom, circleRightBottom);
    painter.drawRoundedRect(circleBounds, circleRadius, circleRadius);

    QPen circlePen(shadow_edge_color_);
    circlePen.setWidth(2);
    circlePen.setCapStyle(Qt::FlatCap);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(circlePen);
    painter.drawArc(circleBounds, 45 * 16, 180 * 16);

    circlePen.setColor(highlight_edge_color_);
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

void TagWidgetCloseButton::enterEvent(QEnterEvent *event) {
    effective_color_ = mouse_over_color_;
    QPushButton::enterEvent(event);
}

void TagWidgetCloseButton::leaveEvent(QEvent *event) {
    effective_color_ = background_color_;
    QPushButton::leaveEvent(event);
}
