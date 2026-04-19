#include "navlibrarycontainer.h"

/*! \brief Constructs an empty NavLibraryContainer.
 *
 * \param parent Optional Qt parent widget.
 */
NavLibraryContainer::NavLibraryContainer(QWidget *parent)
    : WelcomeHintContainer{parent}
{
    setAcceptsClickToDismiss(true);
    setHint(QStringLiteral(":/resources/bookshelf-hint.svg"), 66, 64);
    setAutoFillBackground(false);
    setAutoSortOnRefresh(false);
}

/*! \brief Overrides the Qt base-class mouse-release handler to create a new tag family on click.
 *
 * \param event The mouse release event.
 */
void NavLibraryContainer::mouseReleaseEvent(QMouseEvent *event) {
    // Make a new tag family & tag family widget
    TagFamily* tf = new TagFamily("", nullptr);
    TagFamilyWidget* tw = new TagFamilyWidget(tf, this);
    this->layout()->addWidget(tw);
    tw->startEdit();
}
