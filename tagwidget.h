#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QPainter>
#include "tag.h"

class TagWidget : public QWidget
{
    Q_OBJECT

private:
    Tag* tag_ = new Tag(new TagFamily(),"");

public:
    explicit TagWidget(QWidget *parent = nullptr);

    TagWidget(Tag *tag, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
};

#endif // TAGWIDGET_H
