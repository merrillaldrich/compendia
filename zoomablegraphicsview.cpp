#include "zoomablegraphicsview.h"
#include <QByteArray>
#include <QDataStream>
#include <QMimeData>

/*! \brief Constructs a ZoomableGraphicsView with no initial scene.
 *
 * \param parent Optional Qt parent widget.
 */
ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
}

/*! \brief Constructs a ZoomableGraphicsView bound to the given scene.
 *
 * \param scene  The QGraphicsScene to display.
 * \param parent Optional Qt parent widget.
 */
ZoomableGraphicsView::ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
}

/*! \brief Overrides the Qt base-class wheel handler to perform cursor-centred zoom.
 *
 * \param event The wheel event containing delta and cursor position.
 */
void ZoomableGraphicsView::wheelEvent(QWheelEvent* event)
{
    // Zoom on scroll wheel movement, but limit the lower end of image size
    // to something that sort of fits in the available window

    QRectF itemsRect = scene()->itemsBoundingRect();
    QRectF viewportRect = this->mapToScene(this->viewport()->rect()).boundingRect();

    // Get the current mouse position in the view
    const QPointF mousePos = mapToScene(event->position().toPoint());

    // Zoom in or out based on the delta value

    double effectiveY;

    if( itemsRect.width() < viewportRect.width() && itemsRect.height() < viewportRect.height() ){
        // Prevent zooming out any further if the image is smaller than the viewport
        effectiveY = event->angleDelta().y() > 0 ? event->angleDelta().y() : 0;
    } else {
        // Otherwize allow zoom either way
        effectiveY = event->angleDelta().y();
    }

    if (effectiveY != 0){
        double scaleFactor = ( effectiveY > 0 ) ? 1.15 : 1.0 / 1.15;
        scale(scaleFactor, scaleFactor);
    }

    // Calculate the new mouse position after scaling
    const QPointF newMousePos = mapToScene(event->position().toPoint());

    // Move the scene so the new mouse position is at the same spot as the old one
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (mousePos.x() - newMousePos.x()));
    verticalScrollBar()->setValue(verticalScrollBar()->value() + (mousePos.y() - newMousePos.y()));

    // Accept the event to prevent it from being propagated
    event->accept();
}

/*! \brief Accepts tag drags carrying the custom MIME type and emits tagDragEntered.
 *
 * \param event The drag-enter event.
 */
void ZoomableGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray data = event->mimeData()->data("application/x-dnditemdata");
        QDataStream ds(&data, QIODevice::ReadOnly);
        QString family, tagName;
        QPoint offset;
        ds >> family >> tagName >> offset;
        emit tagDragEntered(family, tagName);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

/*! \brief Emits tagDragMoved with the current scene position as the drag advances.
 *
 * \param event The drag-move event.
 */
void ZoomableGraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
    emit tagDragMoved(mapToScene(event->position().toPoint()));
}

/*! \brief Emits tagDragLeft when the drag leaves the view.
 *
 * \param event The drag-leave event.
 */
void ZoomableGraphicsView::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    emit tagDragLeft();
}

/*! \brief Decodes the payload and emits tagDropped.
 *
 * \param event The drop event.
 */
void ZoomableGraphicsView::dropEvent(QDropEvent *event)
{
    QByteArray data = event->mimeData()->data("application/x-dnditemdata");
    QDataStream ds(&data, QIODevice::ReadOnly);
    QString family, tagName;
    QPoint offset;
    ds >> family >> tagName >> offset;
    emit tagDropped(family, tagName, mapToScene(event->position().toPoint()));
    event->acceptProposedAction();
}
