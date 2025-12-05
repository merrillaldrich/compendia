#include "tagwidget.h"

TagWidget::TagWidget(QWidget *parent)
    : QWidget{parent}
{}

TagWidget::TagWidget(Tag *tag, QWidget *parent)
    : QWidget{parent}
{
    tag_ = tag;
    setMinimumSize(104,25);
    setAttribute(Qt::WA_TranslucentBackground);
    //setAttribute(Qt::WA_TransparentForMouseEvents);
}

void TagWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QPen p = QPen(Qt::NoPen); //QBrush(QColor("black")), 2);
    //painter.setPen(p);

    QBrush b = QBrush(QColor("orange"));
    painter.setPen(p);
    painter.setBrush(b);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawRoundedRect(QRect(2, 2, 100, 24), 12, 12);

    p = QPen(QBrush(QColor("black")), 2);
    painter.setPen(p);
    painter.setFont(QFont("Segoe", 10));
    painter.drawText(QRect(22,6,100,20), Qt::AlignLeft, tag_->tagName);
}

void TagWidget::mousePressEvent(QMouseEvent *event)
{
    //qDebug() << "Drag started!";

    TagWidget *draggedTag = this;

    //qDebug() << draggedTag->tag_->tagName;

    // Render the dragged widget into an image to show what is draggin'
    QPixmap dragimage = this->grab();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << QPoint(event->position().toPoint() - draggedTag->pos());

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(dragimage);
    drag->setHotSpot(event->position().toPoint());
    drag->exec();
    // Do this only if you want to actually move the object on drop. Note this seems to be f'd with by layouts
    //draggedTag->move(event->position().toPoint() - drag->hotSpot());

}

void TagWidget::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "Drag enter event!";

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void TagWidget::dragMoveEvent(QDragMoveEvent *event)
{
    qDebug() << "Drag move event!";

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void TagWidget::dropEvent(QDropEvent *event)
{
    qDebug() << "Drop event!";

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QPixmap pixmap;
        QPoint offset;
        dataStream >> pixmap >> offset;

        qDebug() << "Drop!";

        //QLabel *newIcon = new QLabel(this);
        //newIcon->setPixmap(pixmap);
        //newIcon->move(event->position().toPoint() - offset);
        //newIcon->show();
        //newIcon->setAttribute(Qt::WA_DeleteOnClose);

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

