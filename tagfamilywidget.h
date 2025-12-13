#ifndef TAGFAMILYWIDGET_H
#define TAGFAMILYWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include "flowlayout.h"
#include "tagfamily.h"

class TagFamilyWidget : public QWidget
{
    Q_OBJECT
private:
    TagFamily* tag_family_;

public:
    explicit TagFamilyWidget(QWidget *parent = nullptr);

    TagFamilyWidget(TagFamily *tagFamily, QWidget *parent);

    void paintEvent(QPaintEvent *event);
signals:
};

#endif // TAGFAMILYWIDGET_H
