#include "navlibrarycontainer.h"


NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : TagContainer{parent}{

}

/*! Refresh the navigation area's library panel in the UI.
 *
 *  This is generally used to update the UI to reflect the tags in
 *  the library
 *
 *  \param tags
 *      The set of tags that should be shown
 */

void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    // Make a new tag family & tag family widget
    TagFamily* tf = new TagFamily("", nullptr);
    TagFamilyWidget* tw = new TagFamilyWidget(tf, this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
