#include "navfiltercontainer.h"

/*! \brief Constructs an empty NavFilterContainer.
 *
 * \param parent Optional Qt parent widget.
 */
NavFilterContainer::NavFilterContainer(QWidget *parent)
    : TagContainer{parent}
{

}

/*! \brief Overrides the Qt base-class handler to accept tag drag-enter events.
 *
 * \param event The drag-enter event to evaluate.
 */
void NavFilterContainer::dragEnterEvent(QDragEnterEvent *event)
{
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
void NavFilterContainer::dragMoveEvent(QDragMoveEvent *event)
{
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

/*! \brief Overrides the Qt base-class handler to apply the dropped tag as a filter.
 *
 * \param event The drop event containing the dragged tag data.
 */
void NavFilterContainer::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {

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
        TagFamily* f = t->tagFamily;

        // Identify an existing TagFamilyWidget or make a new TagFamilyWidget if it's missing

        TagWidget* tw = nullptr;
        TagFamilyWidget* tfw = nullptr;

        QList<TagFamilyWidget*> existingTFWidgets = findChildren<TagFamilyWidget*>();

        for(int tfwi = 0; tfwi < existingTFWidgets.count(); ++tfwi){
            TagFamilyWidget* existingTFWidget = existingTFWidgets.at(tfwi);

            if (existingTFWidget->getTagFamily() == f) {
                // Tag family widget exists, use it
                tfw = existingTFWidget;
            }
        }

        if (tfw==nullptr){
            tfw = new TagFamilyWidget(f, this);
            layout()->addWidget(tfw);
            tfw->show();
        }

        // If there are no tag widgets, or there's no tagwidget for the current tag in the current family widget, add a new tagwidget
        QList<TagWidget*> existingTWidgets = tfw->findChildren<TagWidget*>();

        for(int twi = 0; twi < existingTWidgets.count(); ++twi){
            TagWidget* existingTWidget = existingTWidgets.at(twi);
            if(existingTWidget->getTag() == t){
                // Tag widget exists, use it
                tw = existingTWidget;
            }
        }

        if (tw==nullptr){
            tw = new TagWidget(t, tfw);

            if (connect(tw, &TagWidget::deleteRequested, this, &TagContainer::onTagDeleteRequested)){

            } else {
                qWarning() << "Failed to connect tag widget delete to filter container delete";
            }

            tfw->layout()->addWidget(tw);
            tw->show();
        }

        // Apply the dropped tag to the filter
        mainWin->core->addTagFilter(t);

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
