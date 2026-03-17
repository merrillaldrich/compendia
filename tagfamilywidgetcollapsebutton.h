#ifndef TAGFAMILYWIDGETCOLLAPSEBUTTON_H
#define TAGFAMILYWIDGETCOLLAPSEBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include "taggingwidget.h"

/*! \brief A custom collapse/expand button used inside TagFamilyWidget.
 *
 * Draws a small rounded-rectangle button with a painted chevron symbol.  The
 * button colour is derived from the parent TaggingWidget's base_color_ and
 * brightens slightly on mouse-over to give a hover effect.  The chevron points
 * down when the family is expanded and right when it is collapsed.
 */
class TagFamilyWidgetCollapseButton : public QPushButton
{
    Q_OBJECT

public:
    /*! \brief Constructs the button, deriving its colour from the parent TaggingWidget.
     *
     * \param parent The parent widget; expected to be a TaggingWidget subclass.
     */
    explicit TagFamilyWidgetCollapseButton(QWidget *parent = nullptr);

    /*! \brief Sets the collapsed state and repaints the chevron.
     *
     * \param collapsed True if the associated TagFamilyWidget is collapsed.
     */
    void setCollapsed(bool collapsed);

    /*! \brief Returns whether the button is in the collapsed state.
     *
     * \return True if collapsed, false if expanded.
     */
    bool isCollapsed() const;

private:
    QColor background_color_;  ///< Base button colour derived from the parent TaggingWidget.
    QColor mouse_over_color_;  ///< Brightened variant shown on hover.
    QColor effective_color_;   ///< The colour currently being painted (background or hover).
    bool collapsed_ = false;   ///< True when the associated TagFamilyWidget is collapsed.

protected:
    /*! \brief Overrides the Qt base-class paint handler to draw the button and chevron.
     *
     * \param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /*! \brief Overrides the Qt base-class enter-event handler to apply the hover colour.
     *
     * \param event The enter event.
     */
    void enterEvent(QEnterEvent *event) override;

    /*! \brief Overrides the Qt base-class leave-event handler to restore the normal colour.
     *
     * \param event The leave event.
     */
    void leaveEvent(QEvent *event) override;
};

#endif // TAGFAMILYWIDGETCOLLAPSEBUTTON_H
