#include "tagfamilywidget.h"

TagFamilyWidget::TagFamilyWidget(QWidget *parent)
    : QWidget{parent}
{}

TagFamilyWidget::TagFamilyWidget(TagFamily *tagFamily, QWidget *parent)
    : QWidget{parent}
{
    tag_family_ = tagFamily;
    setMinimumSize(304,64);
    setAttribute(Qt::WA_TranslucentBackground);

    FlowLayout* fl = new FlowLayout(this);
    fl->setContentsMargins(4, 28, 4, 4); //left, top, right, bottom
    this->setLayout(fl);

    line_edit_ = new QLineEdit(this);
    line_edit_->move(12, 0);
    line_edit_->hide();

    label_ = new ClickableLabel(this);
    label_->move(18, 0);
    label_->show();

    connect(line_edit_, &QLineEdit::returnPressed, this, &TagFamilyWidget::onReturnPressed);
    connect(label_, &ClickableLabel::clicked, this, &TagFamilyWidget::onLabelClicked);
}

void TagFamilyWidget::mouseReleaseEvent(QMouseEvent *event){

    Tag* t = new Tag(tag_family_, "", this);
    TagWidget* tw = new TagWidget(t, this);
    this->layout()->addWidget(tw);
    tw->startEdit();
    event->accept();

    //QWidget::mouseReleaseEvent(event); // Do not call parent b/c we handled the event fully
}

void TagFamilyWidget::onReturnPressed(){
    endEdit();
}

void TagFamilyWidget::onLabelClicked(QMouseEvent *event){
    startEdit();
    event->accept();
}

void TagFamilyWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen p = QPen(QColor("orange")); //QBrush(QColor("black")), 2);
    painter.setPen(p);
    painter.drawRoundedRect(QRect(2, 4, 300, 52), 12, 12);

    QPainterPath path;
    int radius = 12;

    painter.setBrush(QColor("orange"));
    painter.setPen(Qt::NoPen);

    path.moveTo(radius, 4);
    path.lineTo(width()/3, 4);
    path.lineTo(width()/3, 26);
    path.lineTo(2, 26);
    path.lineTo(2, radius);
    path.arcTo(2, 4, radius*2, radius*2, 180, -90);
    path.closeSubpath();

    painter.drawPath(path);
}

void TagFamilyWidget::startEdit(){
    qDebug() << "Enter family edit mode";
    edit_status_ = "Edit";
    label_->hide();
    line_edit_->show();
    line_edit_->setFocus();
}

void TagFamilyWidget::endEdit(){
    qDebug() << "Leave family edit mode";
    edit_status_ = "Read";
    tag_family_->tagFamilyName = line_edit_->text();
    line_edit_->clearFocus();
    line_edit_->hide();
    label_->setText(tag_family_->tagFamilyName);
    label_->show();
}
