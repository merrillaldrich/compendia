#include "taggablelistview.h"

TaggableListView::TaggableListView(QWidget *parent)
    : QListView{parent}
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
    setDragDropMode(QAbstractItemView::DropOnly);
}

void TaggableListView::dropEvent(QDropEvent *event) {
    // Map the drop position to a model index
    QModelIndex index = indexAt(event->position().toPoint()); // Qt 6

    if (index.isValid()) {
        qDebug() << "Dropped on row:" << index.row()
        << "data:" << index.data().toString();
    } else {
        qDebug() << "Dropped on empty area";
    }

    // Call base implementation to handle the actual drop
    QListView::dropEvent(event);
}

void TaggableListView::dragEnterEvent(QDragEnterEvent *event) {
    qDebug() << "Drag Enter!";
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void TaggableListView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}
