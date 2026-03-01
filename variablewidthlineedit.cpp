#include "variablewidthlineedit.h"

/*! \brief Constructs the line edit and sets an initial minimum width based on font metrics.
 *
 * \param parent Optional Qt parent widget.
 */
VariableWidthLineEdit::VariableWidthLineEdit(QWidget* parent)
    : QLineEdit(parent) {
    QFontMetrics fm = QFontMetrics(font());

    QSizePolicy policy = this->sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Minimum);
    setFixedWidth(fm.horizontalAdvance("WWW") + 14);

}

/*! \brief Resizes the widget to fit the current text content. */
void VariableWidthLineEdit::updateWidth() {
    QFontMetrics fm = QFontMetrics(font());
    setFixedWidth(std::max(fm.horizontalAdvance("WWW"), fm.horizontalAdvance(text())) + 14);
}

/*! \brief Overrides the Qt base-class key-press handler to resize the widget after each keystroke.
 *
 * \param e The key press event.
 */
void VariableWidthLineEdit::keyPressEvent(QKeyEvent *e) {
    // Set the width of the text edit to fit at least 3 W characters, or the physical length of the
    // value, plus the room for padding/decoration of the box itself
    QLineEdit::keyPressEvent(e);
    updateWidth();
}

/*! \brief Returns the minimum size as the size hint so the widget never grows unnecessarily.
 *
 * \return The minimum size.
 */
QSize VariableWidthLineEdit::sizeHint() const {
    return minimumSize();
}
