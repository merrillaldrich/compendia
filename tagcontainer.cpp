#include "tagcontainer.h"

TagContainer::TagContainer(QWidget *parent)
    : QWidget{parent}
{}

void TagContainer::refresh(QSet<Tag*>* tags){

    // Remove existing tag filter widgets
    QLayoutItem* item;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr); // Detach from parent
            widget->deleteLater();      // Schedule deletion
        }
    }

    // Insert all tags in the incoming set, resolving into families
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
            if (! connect(tw, &TagWidget::deleteRequested, this, &TagContainer::onTagDeleteRequested))
                qWarning() << "Failed to connect tag widget delete to container delete";
            w->layout()->addWidget(tw);
            tw->show();

            // Cause the family to grow if there are more tags than space in the family widget
            w->layout()->invalidate();
            w->layout()->activate();
            w->setMinimumHeight(w->childrenRect().height() + 4);
            w->updateGeometry();
        }
    }
}
void TagContainer::onTagDeleteRequested(Tag* tag){

    emit tagDeleteRequested(tag);
}
