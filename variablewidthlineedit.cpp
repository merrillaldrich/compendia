#include "variablewidthlineedit.h"

VariableWidthLineEdit::VariableWidthLineEdit(QWidget* parent)
    : QLineEdit(parent) {
    QFontMetrics fm = QFontMetrics(font());
    setFixedWidth(fm.horizontalAdvance("WWW") + 14);
}

void VariableWidthLineEdit::keyPressEvent(QKeyEvent *e) {
    // Set the width of the text edit to fit at least 3 W characters, or the physical length of the
    // value, plus the room for padding/decoration of the box itself
    QLineEdit::keyPressEvent(e);

    QFontMetrics fm = QFontMetrics(font());
    setFixedWidth(std::max(fm.horizontalAdvance("WWW"),fm.horizontalAdvance(text())) + 14);
}


