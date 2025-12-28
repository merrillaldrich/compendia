#ifndef NAVFILTERCONTAINER_H
#define NAVFILTERCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include "mainwindow.h"
#include "tagcontainer.h"

class NavFilterContainer : public TagContainer
{
    Q_OBJECT
public:
    explicit NavFilterContainer(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

};

#endif // NAVFILTERCONTAINER_H
