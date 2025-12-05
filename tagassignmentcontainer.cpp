#include "tagassignmentcontainer.h"

TagAssignmentContainer::TagAssignmentContainer(QWidget *parent)
    : QWidget{parent}
{}

void TagAssignmentContainer::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "Drag enter event!";

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

void TagAssignmentContainer::dragMoveEvent(QDragMoveEvent *event)
{
    //qDebug() << "Drag move event!";

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

void TagAssignmentContainer::dropEvent(QDropEvent *event)
{
    qDebug() << "Drop event!";

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QPixmap pixmap;
        QPoint offset;
        dataStream >> pixmap >> offset;

        qDebug() << "Drop!";

        //QLabel *newIcon = new QLabel(this);
        //newIcon->setPixmap(pixmap);
        //newIcon->move(event->position().toPoint() - offset);
        //newIcon->show();
        //newIcon->setAttribute(Qt::WA_DeleteOnClose);

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
