#include "previewcontainer.h"
#include <QPainter>
#include <QPen>

// File-local item that draws a rounded rectangle with a cosmetic pen whose corner
// radius is expressed in screen pixels rather than scene units.  This keeps the
// rounding constant at any zoom level, matching the behaviour of the cosmetic pen.
class TagRectItem : public QGraphicsItem
{
    QRectF rect_;
    QPen   pen_;
public:
    TagRectItem(const QRectF &rect, const QPen &pen)
        : rect_(rect), pen_(pen) { setFlag(QGraphicsItem::ItemIsSelectable, false); }

    QRectF boundingRect() const override
    {
        // Cosmetic pens don't consume scene space; add 1-unit padding so Qt's
        // dirty-region tracking includes the stroke.
        return rect_.adjusted(-1, -1, 1, 1);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setPen(pen_);
        painter->setBrush(Qt::NoBrush);

        // Convert the fixed 4-screen-pixel corner radius to scene units using
        // the current horizontal scale of the world transform.
        const qreal screenRadius = 4.0;
        const qreal scaleX = painter->worldTransform().m11();
        const qreal cornerRadius = (scaleX > 0.0) ? screenRadius / scaleX : screenRadius;

        painter->drawRoundedRect(rect_, cornerRadius, cornerRadius);
    }
};

/*! \brief Constructs the preview container and sets up the internal scene and view.
 *
 * \param parent Optional Qt parent widget.
 */
PreviewContainer::PreviewContainer(QWidget *parent)
    : QWidget{parent}
{

    scene = new QGraphicsScene(this);
    view = new ZoomableGraphicsView(scene, this);
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(view);

    //mediaPlayer = new QMediaPlayer(this);
    //videoItem = new QGraphicsVideoItem;
}

/*! \brief Displays the given QImage in the preview pane.
 *
 * \param image The image to display.
 */
void PreviewContainer::preview(QImage image){
    // Replace the image in the scene; scene->clear() deletes all items including overlays
    scene->clear();
    tag_rect_items_.clear();
    image_size_ = image.size();

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene->addItem(item);
    scene->setSceneRect(scene->itemsBoundingRect()); // Prevents incorrect scrollbar extents

    item->setTransformationMode(Qt::SmoothTransformation);

    // Fix up zoom and such
    view->fitInView(item->boundingRect(), Qt::KeepAspectRatio);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->show();
}

/*! \brief Loads and displays the image at the given absolute file path.
 *
 * \param absoluteFilePath Absolute path to the image file to display.
 */
void PreviewContainer::preview(QString absoluteFilePath){

    QImageReader ir(absoluteFilePath);
    ir.setAutoTransform(true);

    // Load the image
    QImage image = ir.read();
    if (image.isNull()) {
        QMessageBox::critical(nullptr, "Error", "Failed to load image: " + ir.errorString());
        //return -1;
    }

    preview(image);
}

/*! \brief Re-fits the displayed image to the viewport if it has not been zoomed in. */
void PreviewContainer::freshen(){

    // Compare the size of the image to the viewport, and if the image is smaller
    // or equal then zoom extents to fill the viewport

    QRectF itemsRect = scene->itemsBoundingRect();
    QRectF viewportRect = view->mapToScene(view->viewport()->rect()).boundingRect();

    if (itemsRect.width() > viewportRect.width() + 30 ||
        itemsRect.height() > viewportRect.height() + 30) {
        // Image is zoomed in, do nothing
    } else {
        // Image is close in size to the viewport, zoom extents to "glue" the edges
        // of the image to the frame and resize with it
        view->fitInView(view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

}

/*! \brief Clears the graphics scene, removing any displayed image. */
void PreviewContainer::clear(){
    if (view->scene() != nullptr){
        view->scene()->clear();
        tag_rect_items_.clear();
        image_size_ = QSizeF();
    }
}

/*! \brief Adds normalised bounding-rectangle overlays on top of the current image.
 *
 * Each rect is given as normalised coordinates in [0.0, 1.0] relative to the image
 * dimensions, paired with the colour to draw its outline.  Any previously set
 * overlays are removed and replaced.
 *
 * \param normalizedRects List of (normalised QRectF, QColor) pairs.
 */
void PreviewContainer::setTagRects(const QList<QPair<QRectF, QColor>> &normalizedRects)
{
    // Delete existing overlay items (QGraphicsItem destructor removes itself from scene)
    for (QGraphicsItem* item : tag_rect_items_)
        delete item;
    tag_rect_items_.clear();

    if (image_size_.isEmpty()) return;

    for (const auto &[normRect, color] : normalizedRects) {
        QRectF sceneRect(
            normRect.x()      * image_size_.width(),
            normRect.y()      * image_size_.height(),
            normRect.width()  * image_size_.width(),
            normRect.height() * image_size_.height()
        );
        QPen pen(color);
        pen.setWidth(4);
        pen.setCosmetic(true);
        TagRectItem* item = new TagRectItem(sceneRect, pen);
        scene->addItem(item);
        tag_rect_items_.append(item);
    }
}

/*! \brief Shows or hides the tag rectangle overlays without removing them from the scene.
 *
 * \param visible True to show overlays, false to hide them.
 */
void PreviewContainer::setTagRectsVisible(bool visible)
{
    for (QGraphicsItem* item : tag_rect_items_)
        item->setVisible(visible);
}
