#ifndef VARIABLEWIDTHLINEEDIT_H
#define VARIABLEWIDTHLINEEDIT_H

#include <QLineEdit>

/*! \brief A QLineEdit that automatically resizes its width to fit its text content.
 *
 * The widget maintains a minimum width equivalent to three 'W' characters and
 * expands as the user types so that the current value is always fully visible.
 */
class VariableWidthLineEdit : public QLineEdit{
    Q_OBJECT

private:
    /*! \brief Overrides the Qt base-class key-press handler to resize the widget after each keystroke.
     *
     * \param e The key press event.
     */
    void keyPressEvent(QKeyEvent *e);

public:
    /*! \brief Constructs the line edit and sets an initial minimum width based on font metrics.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit VariableWidthLineEdit(QWidget* parent = nullptr);

protected:
    /*! \brief Returns the minimum size as the size hint so the widget never grows unnecessarily.
     *
     * \return The minimum size.
     */
    QSize sizeHint() const;

};

#endif // VARIABLEWIDTHLINEEDIT_H
