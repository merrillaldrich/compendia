#ifndef TAGCONTAINER_H
#define TAGCONTAINER_H

#include <QObject>
#include <QWidget>
#include <QLayout>
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
    /*! \brief Comparison helper for sorting TagFamilyWidgets by family name.
     *
     * \param a First TagFamilyWidget to compare.
     * \param b Second TagFamilyWidget to compare.
     * \return True if a's family name is less than b's.
     */
    bool const famNameLessThan(const TagFamilyWidget &a, const TagFamilyWidget &b) ;

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
