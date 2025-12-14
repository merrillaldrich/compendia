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

class TagFamilyWidget : public QWidget
{
    Q_OBJECT

private:
    TagFamily* tag_family_;
    QString edit_status_ = "Read";
    QLineEdit* line_edit_;
    ClickableLabel* label_;

protected:
    void mouseReleaseEvent(QMouseEvent *event);

public:
    explicit TagFamilyWidget(QWidget *parent = nullptr);

    TagFamilyWidget(TagFamily *tagFamily, QWidget *parent);

    void paintEvent(QPaintEvent *event);

    void startEdit();
    void endEdit();

private slots:
    void onReturnPressed();
    void onLabelClicked(QMouseEvent *event);

signals:

};

#endif // TAGFAMILYWIDGET_H
