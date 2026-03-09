#include "tagfamilywidgetcollapsebutton.h"

/*! \brief Constructs the button, deriving its colour from the parent TaggingWidget.
 *
 * \param parent The parent widget; expected to be a TaggingWidget subclass.
 */
TagFamilyWidgetCollapseButton::TagFamilyWidgetCollapseButton(QWidget *parent)
    : QPushButton(parent)
{
    TaggingWidget *p = qobject_cast<TaggingWidget*>(parentWidget());
    if (p) {
        background_color_ = p->base_color_;
    } else {
        background_color_ = QColor::fromHsv(0, 200, 200);
    }

    effective_color_ = background_color_;

    int mouse_over_value_ = background_color_.value() + 12;
    mouse_over_color_ = QColor::fromHsv(background_color_.hue(), background_color_.saturation(), mouse_over_value_);


}

/*! \brief Sets the collapsed state and repaints the chevron.
 *
 * \param collapsed True if the associated TagFamilyWidget is collapsed.
 */
void TagFamilyWidgetCollapseButton::setCollapsed(bool collapsed)
{
    collapsed_ = collapsed;
    update();
}

/*! \brief Returns whether the button is in the collapsed state.
 *
 * \return True if collapsed, false if expanded.
 */
bool TagFamilyWidgetCollapseButton::isCollapsed() const
{
    return collapsed_;
}

/*! \brief Overrides the Qt base-class paint handler to draw the button and chevron.
 *
 * \param event The paint event.
 */
void TagFamilyWidgetCollapseButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    double cx = width() / 2.0;
    double cy = height() / 2.0;
    int circleDiameter = 16;
    int circleRadius = 8;

    // Paint rounded-rect button background, centred on the button centre
    painter.setBrush(effective_color_);
    painter.setPen(Qt::NoPen);
    QRect circleBounds = QRect(qRound(cx) - circleDiameter / 2, qRound(cy) - circleDiameter / 2,
                               circleDiameter, circleDiameter);
    painter.drawRoundedRect(circleBounds, circleRadius, circleRadius);

    // Paint chevron
    double hw = 3.5;  // half-width of the chevron
    double hh = 2.0;  // half-height of the chevron

    QPainterPath chevron;
    chevron.moveTo(cx - hw, cy - hh);
    chevron.lineTo(cx, cy + hh);
    chevron.lineTo(cx + hw, cy - hh);

    QPen chevPen(QColor("#888888"));
    chevPen.setWidth(2);
    chevPen.setCapStyle(Qt::RoundCap);
    chevPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(chevPen);

    if (collapsed_) {
        painter.save();
        painter.translate(cx, cy);
        painter.rotate(-90);
        painter.translate(-cx, -cy);
        painter.drawPath(chevron);
        painter.restore();
    } else {
        painter.drawPath(chevron);
    }
}

/*! \brief Overrides the Qt base-class enter-event handler to apply the hover colour.
 *
 * \param event The enter event.
 */
void TagFamilyWidgetCollapseButton::enterEvent(QEnterEvent *event)
{
    effective_color_ = mouse_over_color_;
    QPushButton::enterEvent(event);
}

/*! \brief Overrides the Qt base-class leave-event handler to restore the normal colour.
 *
 * \param event The leave event.
 */
void TagFamilyWidgetCollapseButton::leaveEvent(QEvent *event)
{
    effective_color_ = background_color_;
    QPushButton::leaveEvent(event);
}
