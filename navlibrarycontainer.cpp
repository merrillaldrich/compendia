#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}

void NavLibraryContainer::refresh(QList<Tag*>* tags){

    // Remove existing tag widgets
    QLayoutItem* item;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr); // Detach from parent
            widget->deleteLater();      // Schedule deletion
        }
    }

    for (int ti=0; ti < tags->count(); ++ti){

        TagFamilyWidget* w = nullptr;
        TagWidget* t = nullptr;

        Tag* currentTag = tags->at(ti);
        TagFamily* currentTagFamily = currentTag->tagFamily;

        // If there are no tag family widgets or there's no tagfamilywidget for the current tag's family, add a new tagfamilywidget
        QList<TagFamilyWidget*> existingTFWidgets = findChildren<TagFamilyWidget*>();

        //for(TagFamilyWidget* tfw : existingTFWidgets){
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
            w->layout()->addWidget(t);
            t->show();
        }
    }
}

void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    // Make a new tag family & tag family widget
    TagFamily* tf = new TagFamily("", nullptr);
    TagFamilyWidget* tw = new TagFamilyWidget(tf, this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
