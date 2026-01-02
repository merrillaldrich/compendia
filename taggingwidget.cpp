#include "taggingwidget.h"

TaggingWidget::TaggingWidget(QWidget *parent)
    : QWidget{parent}{

}

TaggingWidget::TaggingWidget(QColor color, QWidget *parent)
    : QWidget{parent}{

    base_color_ = color;
}

