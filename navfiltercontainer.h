#ifndef NAVFILTERCONTAINER_H
#define NAVFILTERCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include "mainwindow.h"
#include "tagcontainer.h"

/*! \brief Tag container that displays the active filter tags and accepts tag drops to add filters.
 *
 * Extends TagContainer with drag-and-drop support so the user can drag a tag
 * from the library area and drop it here to activate it as a file-list filter.
 */
class NavFilterContainer : public TagContainer
{
    Q_OBJECT
public:
    /*! \brief Constructs an empty NavFilterContainer.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit NavFilterContainer(QWidget *parent = nullptr);

protected:
    /*! \brief Overrides the Qt base-class handler to accept tag drag-enter events.
     *
     * \param event The drag-enter event to evaluate.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /*! \brief Overrides the Qt base-class handler to keep accepting tag drag-move events.
     *
     * \param event The drag-move event to evaluate.
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /*! \brief Overrides the Qt base-class handler to apply the dropped tag as a filter.
     *
     * \param event The drop event containing the dragged tag data.
     */
    void dropEvent(QDropEvent *event) override;

};

#endif // NAVFILTERCONTAINER_H
