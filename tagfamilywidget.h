#ifndef TAGFAMILYWIDGET_H
#define TAGFAMILYWIDGET_H

#include <QWidget>
#include "tagfamily.h"

class TagFamilyWidget : public QWidget
{
    Q_OBJECT
private:
    TagFamily* tag_family_ = new TagFamily("");

public:
    explicit TagFamilyWidget(QWidget *parent = nullptr);

    TagFamilyWidget(TagFamily *tagFamily, QWidget *parent);

signals:
};

#endif // TAGFAMILYWIDGET_H
