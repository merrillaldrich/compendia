#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QObject>
#include <QWheelEvent>
#include <QPointF>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>

/*! \brief A QGraphicsView subclass that supports mouse-wheel zoom centred on the cursor.
 *
 * Scrolling the mouse wheel zooms in or out while keeping the point under the
 * cursor stationary.  Zooming out further than the viewport size is prevented
 * so the image cannot be shrunk to nothing.
 */
class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    /*! \brief Constructs a ZoomableGraphicsView with no initial scene.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

    /*! \brief Constructs a ZoomableGraphicsView bound to the given scene.
     *
     * \param scene  The QGraphicsScene to display.
     * \param parent Optional Qt parent widget.
     */
    ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

signals:
    /*! \brief Emitted once when a tag drag first enters the view.
     *  \param family  Tag family name decoded from the drag payload.
     *  \param tagName Tag name decoded from the drag payload.
     */
    void tagDragEntered(const QString &family, const QString &tagName);

    /*! \brief Emitted while a tag drag moves over the view.
     *  \param scenePos Scene coordinate under the cursor.
     */
    void tagDragMoved(const QPointF &scenePos);

    /*! \brief Emitted when a tag drag leaves the view without dropping. */
    void tagDragLeft();

    /*! \brief Emitted when a tag is dropped on the view.
     *  \param family   Tag family name from the drag payload.
     *  \param tagName  Tag name from the drag payload.
     *  \param scenePos Scene coordinate of the drop point.
     */
    void tagDropped(const QString &family, const QString &tagName, const QPointF &scenePos);

protected:
    /*! \brief Overrides the Qt base-class wheel handler to perform cursor-centred zoom.
     *
     * \param event The wheel event containing delta and cursor position.
     */
    void wheelEvent(QWheelEvent* event) override;

    /*! \brief Accepts tag drags carrying the custom MIME type.
     *
     * \param event The drag-enter event.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /*! \brief Emits tagDragMoved with the current scene position as the drag advances.
     *
     * \param event The drag-move event.
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /*! \brief Emits tagDragLeft when the drag leaves the view.
     *
     * \param event The drag-leave event.
     */
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    /*! \brief Decodes the payload and emits tagDropped.
     *
     * \param event The drop event.
     */
    void dropEvent(QDropEvent *event) override;

    /*! \brief Begins a pan operation when no scene item grabs the press.
     *
     * \param event The mouse press event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /*! \brief Scrolls the view while panning.
     *
     * \param event The mouse move event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /*! \brief Ends the pan operation on button release.
     *
     * \param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool   panning_   = false;
    QPoint pan_start_;
};

#endif // ZOOMABLEGRAPHICSVIEW_H
