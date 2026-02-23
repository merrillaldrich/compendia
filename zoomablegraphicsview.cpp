#include "zoomablegraphicsview.h"

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
