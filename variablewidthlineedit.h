#ifndef VARIABLEWIDTHLINEEDIT_H
#define VARIABLEWIDTHLINEEDIT_H

#include <QLineEdit>

class VariableWidthLineEdit : public QLineEdit{
    Q_OBJECT

private:
    void keyPressEvent(QKeyEvent *e);

public:
    explicit VariableWidthLineEdit(QWidget* parent = nullptr);

};

#endif // VARIABLEWIDTHLINEEDIT_H
