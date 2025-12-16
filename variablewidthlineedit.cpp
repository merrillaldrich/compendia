#include "variablewidthlineedit.h"

VariableWidthLineEdit::VariableWidthLineEdit(QWidget* parent)
    : QLineEdit(parent) {
    setMinimumWidth(50);
    //setMaximumWidth(1000);
}

void VariableWidthLineEdit::keyPressEvent(QKeyEvent *e) {
    if(text().length()>10) {
        QFontMetrics fm = QFontMetrics(font());
        setMinimumWidth(fm.horizontalAdvance(text()) + 24);
    }
    QLineEdit::keyPressEvent(e);
}
