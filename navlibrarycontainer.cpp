#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}

/*! Refresh the navigation area's library panel in the UI.
 *
 *  This is generally used to update the UI to reflect the tags in
 *  the library
 *
 *  \param tags
 *      The set of tags that should be shown
 */
void NavLibraryContainer::refresh(QSet<Tag*>* tags){

    // Remove existing tag widgets
    QLayoutItem* item;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr);
            widget->deleteLater();
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

        if (w == nullptr){
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

        if (t == nullptr){
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
