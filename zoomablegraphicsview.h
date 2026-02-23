#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QObject>
#include <QWheelEvent>
#include <QPointF>
#include <QScrollBar>

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

protected:
    /*! \brief Overrides the Qt base-class wheel handler to perform cursor-centred zoom.
     *
     * \param event The wheel event containing delta and cursor position.
     */
    void wheelEvent(QWheelEvent* event) override;

};

#endif // ZOOMABLEGRAPHICSVIEW_H
