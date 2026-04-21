#include "taggablelistview.h"

/*! \brief Constructs the view and configures it for drop-only drag-and-drop.
 *
 * \param parent Optional Qt parent widget.
 */
TaggableListView::TaggableListView(QWidget *parent)
    : QListView{parent}
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
    setDragDropMode(QAbstractItemView::DropOnly);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

/*! \brief Overrides the Qt base-class handler to apply a dropped tag to the target file(s).
 *
 * \param event The drop event containing the dragged tag data.
 */
void TaggableListView::dropEvent(QDropEvent *event) {
    // Map the drop position to a model index
    QModelIndex index = indexAt(event->position().toPoint());

    if (index.isValid()) {
        // Unpack the mime data from the drop event and convert it to a tagset
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString tagFamilyName;
        QString tagName;
        QPoint offset;

        dataStream >> tagFamilyName >> tagName >> offset;

        TagSet ts = TagSet(tagFamilyName, tagName);

        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());

        if (selectionModel()->isSelected(index)) {
            // Drop on a selected item: apply to every selected file
            const QModelIndexList selected = selectionModel()->selectedIndexes();
            for (const QModelIndex &idx : selected) {
                TaggedFile* tf = idx.data(Qt::UserRole + 1).value<TaggedFile*>();
                mainWin->core->applyTag(tf, ts);
            }
        } else {
            // Drop on an unselected item: apply only to that file
            TaggedFile* tf = index.data(Qt::UserRole + 1).value<TaggedFile*>();
            mainWin->core->applyTag(tf, ts);
        }

        mainWin->refreshTagAssignmentArea();

    } else {
        //qDebug() << "Dropped on empty area";
    }

    // Call base implementation for other QListView actions that
    // might need to happen
    QListView::dropEvent(event);
}

/*! \brief Overrides the Qt base-class handler to accept tag drag-enter events.
 *
 * \param event The drag-enter event to evaluate.
 */
void TaggableListView::dragEnterEvent(QDragEnterEvent *event) {
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

/*! \brief Overrides the Qt base-class handler to keep accepting tag drag-move events.
 *
 * \param event The drag-move event to evaluate.
 */
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
