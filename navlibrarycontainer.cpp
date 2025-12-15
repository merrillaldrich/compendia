#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}

void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    // Make a new tag
    TagFamilyWidget* tw = new TagFamilyWidget(new TagFamily("", nullptr), this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
