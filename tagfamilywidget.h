#ifndef TAGFAMILYWIDGET_H
#define TAGFAMILYWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "flowlayout.h"
#include "tagfamily.h"
#include "tagwidget.h"
#include "clickablelabel.h"
#include "variablewidthlineedit.h"
#include "mainwindow.h"

class TagFamilyWidget : public QWidget
{
    Q_OBJECT

private:
    QString edit_status_ = "Read";
    VariableWidthLineEdit* line_edit_;
    ClickableLabel* label_;
    bool in_library_ = false;
    TagFamily* tag_family_;

protected:
    void mouseReleaseEvent(QMouseEvent *event);

public:
    explicit TagFamilyWidget(QWidget *parent = nullptr);

    TagFamilyWidget(TagFamily *tagFamily, QWidget *parent);

    void paintEvent(QPaintEvent *event);

    void startEdit();
    void endEdit();
    TagFamily* getTagFamily();

private slots:
    void onReturnPressed();
    void onLabelClicked(QMouseEvent *event);
    void onTagFamilyNameChanged();

signals:

};

#endif // TAGFAMILYWIDGET_H
