#include "previewcontainer.h"

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

void PreviewContainer::clear(){
    if (view->scene() != nullptr){
        view->scene()->clear();
    }
}
