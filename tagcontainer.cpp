#include "tagcontainer.h"
#include <QLayout>

/*! \brief Constructs an empty TagContainer.
 *
 * \param parent Optional Qt parent widget.
 */
TagContainer::TagContainer(QWidget *parent)
    : QWidget{parent}
{}

/*! \brief Rebuilds the widget hierarchy to display exactly the given set of tags.
 *
 * \param tags Pointer to the set of Tag pointers to display.
 */
void TagContainer::refresh(QSet<Tag*>* tags){

    // Snapshot collapsed state before destroying existing widgets.
    const QList<TagFamilyWidget*> existing =
        findChildren<TagFamilyWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (TagFamilyWidget *fw : existing)
        collapsed_state_.insert(fw->getTagFamily()->getName(), fw->isCollapsed());

    clear();

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
            connect(tw, &TagWidget::tagNameChanged, this, &TagContainer::tagNameChanged);
            connect(tw, &TagWidget::tagNameChanged, w, [w](Tag*){ w->sort(); });
            connect(tw, &TagWidget::widthChangedDuringEdit, w, &TagFamilyWidget::onChildTagWidthChanged);
            w->layout()->addWidget(tw);
            tw->show();

            // Cause the family to grow if there are more tags than space in the family widget
            w->refreshMinimumHeight();
        }
    }
    // Restore collapsed state for newly created widgets.
    const QList<TagFamilyWidget*> created =
        findChildren<TagFamilyWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (TagFamilyWidget *fw : created) {
        const QString name = fw->getTagFamily()->getName();
        if (collapsed_state_.contains(name))
            fw->setCollapsed(collapsed_state_.value(name));
    }

    this->sort();
}

/*! \brief Removes all child TagFamilyWidget and TagWidget items from the layout. */
void TagContainer::clear(){
    // Remove existing tag filter widgets
    QLayoutItem* item;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->hide();             // Prevent flash: setParent(nullptr) makes a top-level window
            widget->setParent(nullptr); // Detach from parent
            widget->deleteLater();      // Schedule deletion
        }
    }

}

/*! \brief Forwards a tag-delete request up to the tagDeleteRequested signal.
 *
 * \param tag The Tag whose deletion was requested.
 */
void TagContainer::onTagDeleteRequested(Tag* tag){
    emit tagDeleteRequested(tag);
}

/*! \brief Shows only families and tags whose tag name starts with \p text. */
void TagContainer::filter(const QString &text) {
    const QList<TagFamilyWidget*> families =
        findChildren<TagFamilyWidget*>(Qt::FindDirectChildrenOnly);

    for (TagFamilyWidget *fw : families) {
        const QList<TagWidget*> tags =
            fw->findChildren<TagWidget*>(Qt::FindDirectChildrenOnly);

        bool anyVisible = false;
        for (TagWidget *tw : tags) {
            const bool matches = text.isEmpty() ||
                tw->getTag()->getName().contains(text, Qt::CaseInsensitive);
            tw->setVisible(matches);
            if (matches)
                anyVisible = true;
        }

        const bool showFamily = text.isEmpty() || anyVisible;
        fw->setVisible(showFamily);

        if (showFamily) {
            fw->layout()->invalidate();
            fw->layout()->activate();
            const int needed = qMax(64, fw->layout()->heightForWidth(fw->contentsRect().width()) + 4);
            fw->setMinimumHeight(needed);
            fw->updateGeometry();
        }
    }

    layout()->invalidate();
    layout()->activate();
    updateGeometry();
}

/*! \brief Sorts all TagFamilyWidget children alphabetically by family name,
 *  and sorts tags within each family alphabetically by tag name. */
void TagContainer::sort() {
    QList<TagFamilyWidget*> fwlist;
    QLayoutItem* item = nullptr;

    // Temporarily move child widgets to a list
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            if (auto *tfw = qobject_cast<TagFamilyWidget*>(widget)) {
                fwlist.append(tfw);
            }
        }
        delete item; // is this needed to avoid QLayoutItem leak?
    }

    // Sort the list (compare pointers)
    std::sort(fwlist.begin(), fwlist.end(),
              [](TagFamilyWidget* a, TagFamilyWidget* b) {
                  return a->getTagFamily()->getName() < b->getTagFamily()->getName();
              });

    // Put the widgets back in the layout, in order
    for (TagFamilyWidget* w : fwlist) {
        w->sort();
        layout()->addWidget(w);
        w->refreshMinimumHeight();
    }
}
