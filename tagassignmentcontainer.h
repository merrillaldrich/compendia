#ifndef TAGASSIGNMENTCONTAINER_H
#define TAGASSIGNMENTCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLayout>
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "mainwindow.h"
#include "tagcontainer.h"

class TagAssignmentContainer : public TagContainer
{
    Q_OBJECT
public:
    explicit TagAssignmentContainer(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void onTagDeleteRequested(Tag* tag);

signals:
    void tagDeleteRequested(Tag* tag);
};

#endif // TAGASSIGNMENTCONTAINER_H
