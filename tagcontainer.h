#ifndef TAGCONTAINER_H
#define TAGCONTAINER_H

#include <QObject>
#include <QWidget>
#include <QLayout>
#include <QList>
#include <QMap>
#include <QSet>
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "tag.h"

/*! \brief Base container widget that displays a set of tags grouped into TagFamilyWidget items.
 *
 * TagContainer manages a FlowLayout of TagFamilyWidget children, each of which
 * holds one or more TagWidget children.  Subclasses (NavFilterContainer,
 * NavLibraryContainer, TagAssignmentContainer) add drag-and-drop or mouse
 * interaction behaviour on top of this base.
 */
class TagContainer : public QWidget
{
    Q_OBJECT

private:
    /// Collapsed state keyed by family name; persisted across refresh() calls.
    QMap<QString, bool> collapsed_state_;

    /// Display order for tag families; new families append at end.
    QList<TagFamily*> family_order_;

    /// Display order for tags within each family; new tags append at end.
    QMap<TagFamily*, QList<Tag*>> tag_order_;

    /// When true, refresh() sorts alphabetically; when false, insertion order is preserved.
    bool autoSortOnRefresh_ = true;

    bool const famNameLessThan(const TagFamilyWidget &a, const TagFamilyWidget &b);

public:
    /*! \brief Constructs an empty TagContainer.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TagContainer(QWidget *parent = nullptr);

    /*! \brief Rebuilds the widget hierarchy to display exactly the given set of tags.
     *
     * \param tags Pointer to the set of Tag pointers to display.
     */
    void refresh(QSet<Tag*>* tags);

    /*! \brief Removes all child TagFamilyWidget and TagWidget items from the layout. */
    void clear();

    /*! \brief Forwards a tag-delete request up to the tagDeleteRequested signal.
     *
     * \param tag The Tag whose deletion was requested.
     */
    void onTagDeleteRequested(Tag* tag);

    /*! \brief Sorts all TagFamilyWidget children alphabetically by family name,
     *  and sorts tags within each family alphabetically by tag name. */
    void sort();

    /*! \brief Controls whether refresh() auto-sorts; pass false to preserve insertion order. */
    void setAutoSortOnRefresh(bool autoSort);

    /*! \brief Shows only tag families and tags whose name starts with \p text.
     *
     * When \p text is empty all tags and families are shown.  Otherwise each
     * TagWidget is shown only if its tag name contains \p text
     * (case-insensitive), and a TagFamilyWidget is shown only when at least
     * one of its child TagWidgets is visible.
     *
     * \param text The prefix to filter on.
     */
    void filter(const QString &text);

protected:

signals:
    /*! \brief Emitted when the user requests removal of a tag from this container.
     *
     * \param tag The Tag to be removed.
     */
    void tagDeleteRequested(Tag* tag);

    /*! \brief Emitted when any tag displayed in this container has its name changed.
     *
     * Forwarded from the TagWidget::tagNameChanged signal of each child widget
     * created during refresh().  Allows MainWindow to update the preview overlay
     * labels without polling.
     *
     * \param tag The Tag whose name was changed.
     */
    void tagNameChanged(Tag* tag);
};

#endif // TAGCONTAINER_H
