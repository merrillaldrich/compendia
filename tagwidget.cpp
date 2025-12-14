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

    line_edit_ = new QLineEdit(this);
    line_edit_->move(12, 2);
    line_edit_->hide();

    label_ = new ClickableLabel(this);
    label_->move(18, 4);
    label_->show();

    connect(line_edit_, &QLineEdit::returnPressed, this, &TagWidget::onReturnPressed);
    connect(label_, &ClickableLabel::clicked, this, &TagWidget::onLabelClicked);

}

void TagWidget::onReturnPressed(){
    qDebug() << line_edit_->text();
    endEdit();
}

void TagWidget::onLabelClicked(QMouseEvent *event){
    qDebug() << "Label clicked!";
    startEdit();
    event->accept();
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
}

void TagWidget::startEdit(){
    qDebug() << "Enter tag edit mode";
    edit_status_ = "Edit";
    label_->hide();
    line_edit_->show();
    line_edit_->setFocus();
}

void TagWidget::endEdit(){
    qDebug() << "Leave tag edit mode";
    edit_status_ = "Read";
    tag_->tagName = line_edit_->text();
    line_edit_->clearFocus();
    line_edit_->hide();
    label_->setText(tag_->tagName);
    label_->show();
}

void TagWidget::mousePressEvent(QMouseEvent *event)
{
    //qDebug() << "Drag started!";

    TagWidget* draggedTag = this;
    Tag* t = draggedTag->tag_;
    QString tagName = t->tagName;
    QString tagFamilyName = t->tagFamily->tagFamilyName;

    //qDebug() << draggedTag->tag_->tagName;

    // Render the dragged widget into an image to show what is draggin'
    QPixmap dragimage = this->grab();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << tagFamilyName << tagName << QPoint(event->position().toPoint() - draggedTag->pos());

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);

    // Bundle the image and mime data into a QDrag object
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(dragimage);
    drag->setHotSpot(event->position().toPoint());
    drag->exec();
    // Do this only if you want to actually move the object on drop. Note this seems to be f'd with by layouts:
    //draggedTag->move(event->position().toPoint() - drag->hotSpot());

}


