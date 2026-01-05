#include "previewcontainer.h"

PreviewContainer::PreviewContainer(QWidget *parent)
    : QWidget{parent}
{

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
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
    scene->setSceneRect(scene->itemsBoundingRect());

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
    if (view->scene() != nullptr){
        view->fitInView(view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void PreviewContainer::clear(){
    if (view->scene() != nullptr){
        view->scene()->clear();
    }
}
