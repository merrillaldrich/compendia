#ifndef TAGASSIGNMENTCONTAINER_H
#define TAGASSIGNMENTCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLayout>
#include "tagwidget.h"
#include "mainwindow.h"

class TagAssignmentContainer : public QWidget
{
    Q_OBJECT
public:
    explicit TagAssignmentContainer(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
};

#endif // TAGASSIGNMENTCONTAINER_H
