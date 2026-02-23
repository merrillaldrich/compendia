#include "previewcontainer.h"

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
    // Replace the image in the scene
    scene->clear();

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
    }
}
