#include "zoomablegraphicsview.h"

ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
}
ZoomableGraphicsView::ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent* event)
{
    // Get the current mouse position in the view
    const QPointF mousePos = mapToScene(event->position().toPoint());

    // Zoom in or out based on the delta value
    double scaleFactor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    scale(scaleFactor, scaleFactor);

    // Calculate the new mouse position after scaling
    const QPointF newMousePos = mapToScene(event->position().toPoint());

    // Move the scene so the new mouse position is at the same spot as the old one
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (mousePos.x() - newMousePos.x()));
    verticalScrollBar()->setValue(verticalScrollBar()->value() + (mousePos.y() - newMousePos.y()));

    // We must accept the event to prevent it from being propagated
    event->accept();
}
