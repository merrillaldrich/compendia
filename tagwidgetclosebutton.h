#ifndef TAGWIDGETCLOSEBUTTON_H
#define TAGWIDGETCLOSEBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include "taggingwidget.h"

/*! \brief A custom circular close button used inside TagWidget.
 *
 * Draws a small rounded-rectangle button with a painted 'x' symbol.  The
 * button colour is derived from the parent TaggingWidget's base_color_ and
 * brightens slightly on mouse-over to give a hover effect.
 */
class TagWidgetCloseButton : public QPushButton
{
    Q_OBJECT

public:
    /*! \brief Constructs the button, deriving its colour from the parent TaggingWidget.
     *
     * \param parent The parent widget; expected to be a TaggingWidget subclass.
     */
    explicit TagWidgetCloseButton(QWidget *parent = nullptr);

private:
    QColor background_color_;       ///< Base button colour derived from the parent TaggingWidget.
    QColor highlight_edge_color_;   ///< Light edge colour for the 3-D bevel effect.
    QColor shadow_edge_color_;      ///< Dark edge colour for the 3-D bevel effect.
    QColor mouse_over_color_;       ///< Brightened variant shown on hover.
    QColor effective_color_;        ///< The colour currently being painted (background or hover).

protected:
    /*! \brief Overrides the Qt base-class paint handler to draw the circular button and 'x'.
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

#endif // TAGWIDGETCLOSEBUTTON_H
