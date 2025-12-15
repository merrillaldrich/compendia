#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : QWidget{parent}{

}

void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    // Make a new tag family & tag family widget
    TagFamily* tf = new TagFamily("", nullptr);
    TagFamilyWidget* tw = new TagFamilyWidget(tf, this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
