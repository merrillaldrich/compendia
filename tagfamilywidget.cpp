#include "tagfamilywidget.h"

TagFamilyWidget::TagFamilyWidget(QWidget *parent)
    : QWidget{parent}
{}

TagFamilyWidget::TagFamilyWidget(TagFamily *tagFamily, QWidget *parent)
    : QWidget{parent}
{
    tag_family_ = tagFamily;
    setMinimumSize(304,56);
    setAttribute(Qt::WA_TranslucentBackground);

    FlowLayout* fl = new FlowLayout(this);
    fl->setContentsMargins(4, 24, 4, 4); //left, top, right, bottom
    this->setLayout(fl);
}

void TagFamilyWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen p = QPen(QColor("orange")); //QBrush(QColor("black")), 2);
    painter.setPen(p);

    //QBrush b = QBrush(QColor("orange"));
    //painter.setPen(p);
    //painter.setBrush(b);

    painter.drawRoundedRect(QRect(2, 2, 300, 52), 12, 12);

    QPainterPath path;
    int radius = 12;

    painter.setBrush(QColor("orange")); // light blue
    painter.setPen(Qt::NoPen);

    path.moveTo(radius, 2);
    path.lineTo(width()/3, 2);
    path.lineTo(width()/3, 24);
    path.lineTo(2, 24);
    path.lineTo(2, radius);
    path.arcTo(2, 2, radius*2, radius*2, 180, -90);
    path.closeSubpath();

    painter.drawPath(path);

    p = QPen(QBrush(QColor("black")), 2);
    painter.setPen(p);
    painter.setFont(QFont("Segoe", 10));
    painter.drawText(QRect(22,6,100,20), Qt::AlignLeft, tag_family_->tagFamilyName);
}
