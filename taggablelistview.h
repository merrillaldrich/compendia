#ifndef TAGGABLELISTVIEW_H
#define TAGGABLELISTVIEW_H

#include <QListView>
#include <QDropEvent>
#include <QMimeData>
#include "mainwindow.h"
#include "taggedfile.h"
#include "tagset.h"

/*! \brief A QListView subclass that accepts tag drops to apply tags to media files.
 *
 * Displays the filtered list of media files and handles drag-and-drop of tag
 * widgets onto file items.  When a tag is dropped onto a selected item, it is
 * applied to all currently selected files; when dropped onto an unselected item
 * it is applied only to that one file.
 */
class TaggableListView : public QListView
{
public:
    /*! \brief Constructs the view and configures it for drop-only drag-and-drop.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TaggableListView(QWidget *parent = nullptr);

protected:
    /*! \brief Overrides the Qt base-class handler to apply a dropped tag to the target file(s).
     *
     * \param event The drop event containing the dragged tag data.
     */
    void dropEvent(QDropEvent *event) override;

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

};

#endif // TAGGABLELISTVIEW_H
