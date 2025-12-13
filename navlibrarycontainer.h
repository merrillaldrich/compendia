#ifndef NAVLIBRARYCONTAINER_H
#define NAVLIBRARYCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "tag.h"
#include "tagfamily.h"
#include "tagwidget.h"

class NavLibraryContainer : public QWidget
{
    Q_OBJECT
public:
    explicit NavLibraryContainer(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

signals:
};

#endif // NAVLIBRARYCONTAINER_H
