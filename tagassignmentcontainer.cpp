#include "tagassignmentcontainer.h"

TagAssignmentContainer::TagAssignmentContainer(QWidget *parent)
    : QWidget{parent}
{}

/*! Update the tag assignment area to show only the tags in the passed list.
 *
 *  @brief Adjust the UI to display only the tags assigned to the files in the
 *  filtered list of files
 *
 *  @param tags Set of tags to include in the assignment area
 */
void TagAssignmentContainer::refresh(QSet<Tag*>* tags){

    // Remove existing tag assignment widgets
    QLayoutItem* item;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr); // Detach from parent
            widget->deleteLater();      // Schedule deletion
        }
    }

    QSetIterator<Tag *> i(*tags);
    while (i.hasNext()) {
        Tag* currentTag = i.next();
        TagFamily* currentTagFamily = currentTag->tagFamily;

        TagFamilyWidget* w = nullptr;
        TagWidget* t = nullptr;

        // If there are no tag family widgets or there's no tagfamilywidget for the current tag's family, add a new tagfamilywidget
        QList<TagFamilyWidget*> existingTFWidgets = findChildren<TagFamilyWidget*>();

        for(int tfwi = 0; tfwi < existingTFWidgets.count(); ++tfwi){
            TagFamilyWidget* existingTFWidget = existingTFWidgets.at(tfwi);

            if (existingTFWidget->getTagFamily() == currentTagFamily) {
                // Tag family widget exists, use it
                w = existingTFWidget;
            }
        }

        if (w==nullptr){
            w = new TagFamilyWidget(currentTag->tagFamily, this);
            layout()->addWidget(w);
            w->show();
        }

        // If there are no tag widgets, or there's no tagwidget for the current tag in the current family widget, add a new tagwidget
        QList<TagWidget*> existingTWidgets = w->findChildren<TagWidget*>();

        for(int twi = 0; twi < existingTWidgets.count(); ++twi){
            TagWidget* existingTWidget = existingTWidgets.at(twi);
            if(existingTWidget->getTag() == currentTag){
                // Tag widget exists, use it
                t = existingTWidget;
            }
        }

        if (t==nullptr){
            t = new TagWidget(currentTag, w);
            connect(t, &TagWidget::deleteRequested, this, &onTagDeleteRequested);
            w->layout()->addWidget(t);
            t->show();
        }
    }
}

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
            tfw->layout()->addWidget(tw);
            tw->show();
        }

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

void TagAssignmentContainer::onTagDeleteRequested(Tag* tag){
    // Tell core to remove this tag from all the files in the filtered file list

    emit tagDeleteRequested(tag);
}



