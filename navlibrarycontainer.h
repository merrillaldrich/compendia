#ifndef NAVLIBRARYCONTAINER_H
#define NAVLIBRARYCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "tag.h"
#include "tagfamily.h"
#include "tagfamilywidget.h"
#include "tagcontainer.h"

/*! \brief Tag container that displays the full tag library and allows new families to be created.
 *
 * Extends TagContainer so that clicking on an empty area of the container
 * creates a new TagFamily and puts its widget into inline-edit mode immediately.
 */
class NavLibraryContainer : public TagContainer
{
    Q_OBJECT
public:
    /*! \brief Constructs an empty NavLibraryContainer.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit NavLibraryContainer(QWidget *parent = nullptr);

protected:
    /*! \brief Overrides the Qt base-class mouse-release handler to create a new tag family on click.
     *
     * \param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
};

#endif // NAVLIBRARYCONTAINER_H
