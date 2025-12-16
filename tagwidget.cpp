#include "tagwidget.h"

TagWidget::TagWidget(QWidget *parent)
    : QWidget{parent}
{}

TagWidget::TagWidget(Tag *tag, QWidget *parent)
    : QWidget{parent}{
    tag_ = tag;

    // Connect this widget to its tag's name changed event
    connect(tag_, &Tag::nameChanged, this, &onTagNameChanged);

    setMinimumSize(104,28);
    setAttribute(Qt::WA_TranslucentBackground);

    line_edit_ = new QLineEdit(this);
    line_edit_->move(12, 0);
    line_edit_->setFixedHeight(line_edit_->height());
    line_edit_->hide();

    label_ = new ClickableLabel(this);
    label_->move(18, 0);
    label_->setFixedHeight(label_->height());
    label_->setText(tag->getName());
    label_->adjustSize();
    setMinimumWidth(label_->width() + 32);
    label_->show();

    connect(line_edit_, &QLineEdit::returnPressed, this, &TagWidget::onReturnPressed);
    connect(label_, &ClickableLabel::clicked, this, &TagWidget::onLabelClicked);
}

void TagWidget::onReturnPressed(){
    endEdit();
}

void TagWidget::onLabelClicked(QMouseEvent *event){
    qDebug() << "Tag label clicked";
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

    int inset = 2;
    int leftX = inset;
    int topY = inset;
    int rightX = width() - (inset * 2);
    int bottomY = height() - (inset * 2);
    int cornerRadius = 12;

    painter.drawRoundedRect(QRect(leftX, topY, rightX, bottomY), cornerRadius, cornerRadius);
}

void TagWidget::startEdit(){
    qDebug() << "Enter tag edit mode";
    edit_status_ = "Edit";
    label_->hide();
    line_edit_->setText(tag_->getName());
    line_edit_->show();
    line_edit_->setFocus();
}

void TagWidget::endEdit(){
    qDebug() << "Leave tag edit mode";
    edit_status_ = "Read";
    tag_->setName(line_edit_->text());

    // If this tag isn't represented in the library then add it.
    // Should happen once on creation; After that edits already apply
    // in the library because this is holding a pointer

    if(! in_library_) {
        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());
        mainWin->core->addLibraryTag(tag_);
        in_library_ = true;
    }

    line_edit_->clearFocus();
    line_edit_->hide();
    label_->setText(tag_->getName());
    label_->show();
}

void TagWidget::mousePressEvent(QMouseEvent *event){

    // To determine if this should be a drag operation or just a click, record the location of the mouse when left button pressed
    // then use the value in MouseMoveEvent() to check the distance the mouse was moved

    if (event->button() == Qt::LeftButton){
        drag_start_position_ = event->pos();
    }

}

void TagWidget::mouseMoveEvent(QMouseEvent *event){

    // Start a drag, but only if the mouse was moved with the left button pressed

    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - drag_start_position_).manhattanLength() < QApplication::startDragDistance())
        return;

    TagWidget* draggedTag = this;
    Tag* t = draggedTag->tag_;
    QString tagName = t->getName();
    QString tagFamilyName = t->tagFamily->getName();

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

void TagWidget::onTagNameChanged(){
    label_->setText(tag_->getName());
    label_->adjustSize(); // Note this is only changing the width, height is fixed with a policy
    setMinimumWidth(label_->width() + 32);
}

Tag* TagWidget::getTag(){
    return tag_;
}

