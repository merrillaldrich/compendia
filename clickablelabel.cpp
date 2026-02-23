#include "clickablelabel.h"

/*! \brief Constructs the label and sets the cursor to a pointing hand.
 *
 * \param parent Optional Qt parent widget.
 */
ClickableLabel::ClickableLabel(QWidget* parent)
    : QLabel(parent) {
    setCursor(Qt::PointingHandCursor);
}

/*! \brief Overrides the Qt base-class handler to emit the clicked() signal.
 *
 * \param event The mouse release event.
 */
void ClickableLabel::mouseReleaseEvent(QMouseEvent* event) {
    emit clicked(event);
    //QLabel::mousePressEvent(event);
}
