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
    QModelIndex index = indexAt(event->position().toPoint());

    if (index.isValid()) {
        //qDebug() << "Dropped on row:" << index.row()
        //<< "data:" << index.data().toString();

        // Assign the dropped tag to the item

        // Retrieve the file associated with the item at the drop location
        //QStandardItem* item = index.model()->data(index);
        QVariant var = index.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

        // Unpack the mime data from the drop event and convert it to a tagset
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString tagFamilyName;
        QString tagName;
        QPoint offset;

        dataStream >> tagFamilyName >> tagName >> offset;

        TagSet ts = TagSet(tagFamilyName, tagName);

        // Apply the tagset to the item
        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());
        mainWin->core->applyTag(itemAsTaggedFile, ts);

    } else {
        //qDebug() << "Dropped on empty area";
    }

    // Call base implementation for other QListView actions that
    // might need to happen
    QListView::dropEvent(event);
}

void TaggableListView::dragEnterEvent(QDragEnterEvent *event) {
    //qDebug() << "Drag Enter!";
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
