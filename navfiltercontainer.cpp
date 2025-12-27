#include "navfiltercontainer.h"

NavFilterContainer::NavFilterContainer(QWidget *parent)
    : QWidget{parent}
{

}

void NavFilterContainer::refresh(QSet<Tag*>* tags){

    // Remove existing tag filter widgets
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
        TagWidget* tw = nullptr;

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
                tw = existingTWidget;
            }
        }

        if (tw==nullptr){
            tw = new TagWidget(currentTag, w);
            if (! connect(tw, &TagWidget::deleteRequested, this, &NavFilterContainer::onTagDeleteRequested))
                qWarning() << "Failed to connect tag widget delete to filter container delete";
            w->layout()->addWidget(tw);
            tw->show();
        }
    }
}

void NavFilterContainer::dragEnterEvent(QDragEnterEvent *event)
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

void NavFilterContainer::dragMoveEvent(QDragMoveEvent *event)
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

void NavFilterContainer::dropEvent(QDropEvent *event)
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

            if (connect(tw, &TagWidget::deleteRequested, this, &NavFilterContainer::onTagDeleteRequested)){

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

void NavFilterContainer::onTagDeleteRequested(Tag* tag){
    // Tell core to remove this tag from all the files in the filtered file list

    emit tagDeleteRequested(tag);
}

