#include "tagfamilywidget.h"

TagFamilyWidget::TagFamilyWidget(QWidget *parent)
    : QWidget{parent}
{}

TagFamilyWidget::TagFamilyWidget(TagFamily *tagFamily, QWidget *parent)
    : QWidget{parent}
{
    tag_family_ = tagFamily;
    setMinimumSize(108,30);
    setAttribute(Qt::WA_TranslucentBackground);
    //setAttribute(Qt::WA_TransparentForMouseEvents);
}
