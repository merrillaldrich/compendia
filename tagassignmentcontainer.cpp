#include "tagassignmentcontainer.h"

TagAssignmentContainer::TagAssignmentContainer(QWidget *parent)
    : QWidget{parent}
{}

void TagAssignmentContainer::dragEnterEvent(QDragEnterEvent *event)
{
    //qDebug() << "Drag enter event!";

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
    //qDebug() << "Drop event!";

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {

        //qDebug() << "Drop!";

        // Convert the dropped data
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString tagFamilyName;
        QString tagName;
        QPoint offset;

        dataStream >> tagFamilyName >> tagName >> offset;

        // Locate the dropped tag in the tag library
        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());

        Tag* t = mainWin->core->getTag(tagFamilyName, tagName);

        TagWidget* droppedTagWidget = new TagWidget(t,this);
        this->layout()->addWidget(droppedTagWidget);
        droppedTagWidget->show();

        // Apply the dropped tag to all visible files
        mainWin->core->applyTag(t);

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
