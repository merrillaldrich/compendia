#include "tagcontainer.h"
#include <QLayout>
#include <algorithm>

/*! \brief Constructs an empty TagContainer.
 *
 * \param parent Optional Qt parent widget.
 */
TagContainer::TagContainer(QWidget *parent)
    : QWidget{parent}
{}

/*! \brief Controls whether refresh() auto-sorts alphabetically on every call.
 *
 * Set to false on the library container so insertion order is preserved.
 * Defaults to true so assignment and filter containers stay alphabetical.
 */
void TagContainer::setAutoSortOnRefresh(bool autoSort)
{
    autoSortOnRefresh_ = autoSort;
}

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

    // Build lookup: family -> its tags in this refresh pass.
    QMap<TagFamily*, QList<Tag*>> incoming;
    for (Tag* tag : *tags)
        incoming[tag->tagFamily].append(tag);

    // Update family_order_: remove absent families, append new ones at the end.
    family_order_.removeIf([&incoming](TagFamily* f){ return !incoming.contains(f); });
    for (TagFamily* fam : incoming.keys()) {
        if (!family_order_.contains(fam))
            family_order_.append(fam);
    }

    // Update tag_order_ for each tracked family.
    for (TagFamily* fam : family_order_) {
        QList<Tag*>& order = tag_order_[fam];
        const QList<Tag*>& incomingTags = incoming[fam];
        order.removeIf([&incomingTags](Tag* t){ return !incomingTags.contains(t); });
        for (Tag* tag : incomingTags) {
            if (!order.contains(tag))
                order.append(tag);
        }
    }
    // Drop stale family entries from tag_order_.
    for (TagFamily* fam : tag_order_.keys()) {
        if (!family_order_.contains(fam))
            tag_order_.remove(fam);
    }

    // Sort tracking lists alphabetically when auto-sort is on.
    if (autoSortOnRefresh_) {
        std::sort(family_order_.begin(), family_order_.end(),
                  [](TagFamily* a, TagFamily* b){ return a->getName() < b->getName(); });
        for (QList<Tag*>& tagList : tag_order_)
            std::sort(tagList.begin(), tagList.end(),
                      [](Tag* a, Tag* b){ return a->getName() < b->getName(); });
    }

    // Build widgets in tracked order.
    for (TagFamily* fam : family_order_) {
        TagFamilyWidget* w = new TagFamilyWidget(fam, this);
        layout()->addWidget(w);
        w->show();

        for (Tag* tag : tag_order_[fam]) {
            TagWidget* tw = new TagWidget(tag, w);
            if (!connect(tw, &TagWidget::deleteRequested, this, &TagContainer::onTagDeleteRequested))
                qWarning() << "Failed to connect tag widget delete to container delete";
            connect(tw, &TagWidget::tagNameChanged, this, &TagContainer::tagNameChanged);
            if (autoSortOnRefresh_)
                connect(tw, &TagWidget::tagNameChanged, w, [w](Tag*){ w->sort(); });
            connect(tw, &TagWidget::widthChangedDuringEdit, w, &TagFamilyWidget::onChildTagWidthChanged);
            w->layout()->addWidget(tw);
            tw->show();
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

/*! \brief Shows only families and tags whose tag name contains \p text (case-insensitive); shows all when empty. */
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
    // Sort tracking lists alphabetically.
    std::sort(family_order_.begin(), family_order_.end(),
              [](TagFamily* a, TagFamily* b){ return a->getName() < b->getName(); });
    for (QList<Tag*>& tagList : tag_order_)
        std::sort(tagList.begin(), tagList.end(),
                  [](Tag* a, Tag* b){ return a->getName() < b->getName(); });

    // Pull all TagFamilyWidgets out of the layout.
    QList<TagFamilyWidget*> fwlist;
    QLayoutItem* item = nullptr;
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget())
            if (auto *tfw = qobject_cast<TagFamilyWidget*>(widget))
                fwlist.append(tfw);
        delete item;
    }

    // Re-insert them in the order dictated by the now-sorted family_order_.
    for (TagFamily* fam : family_order_) {
        for (TagFamilyWidget* w : fwlist) {
            if (w->getTagFamily() == fam) {
                w->sort();
                layout()->addWidget(w);
                w->refreshMinimumHeight();
                break;
            }
        }
    }
}
