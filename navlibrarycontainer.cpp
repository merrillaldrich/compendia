#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}


void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    qDebug() << "Library Mouse Click";

    // Make a new tag
    //TagWidget* tw = new TagWidget(new Tag(new TagFamily("Foo", nullptr),"Bar", nullptr), this);

    TagFamilyWidget* tw = new TagFamilyWidget(new TagFamily("Default", nullptr), this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
