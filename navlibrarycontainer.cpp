#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}


void NavLibraryContainer::mousePressEvent(QMouseEvent *event) {
    qDebug() << "Library Mouse Press";

    // Make a new tag
    TagWidget* tw = new TagWidget(new Tag(new TagFamily("Foo", nullptr),"Bar", nullptr), this);
    this->layout()->addWidget(tw);
}
