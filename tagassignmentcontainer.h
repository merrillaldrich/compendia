#ifndef TAGASSIGNMENTCONTAINER_H
#define TAGASSIGNMENTCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLayout>
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "mainwindow.h"
#include "welcomehintcontainer.h"

/*! \brief Tag container showing tags assigned to the current file selection, accepting tag drops.
 *
 * Extends WelcomeHintContainer with drag-and-drop support so the user can drag a tag
 * from the library area and drop it here to apply it to all currently visible
 * (filtered) files.  Before any folder is loaded, a dismissible welcome hint is shown;
 * dropping a tag onto it reveals the container and applies the tag to all visible files.
 */
class TagAssignmentContainer : public WelcomeHintContainer
{
    Q_OBJECT
public:
    /*! \brief Constructs an empty TagAssignmentContainer.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TagAssignmentContainer(QWidget *parent = nullptr);

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

    /*! \brief Overrides the Qt base-class handler to apply the dropped tag to all filtered files.
     *
     * \param event The drop event containing the dragged tag data.
     */
    void dropEvent(QDropEvent *event) override;

};

#endif // TAGASSIGNMENTCONTAINER_H
