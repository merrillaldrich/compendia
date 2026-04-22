#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>

/*! \brief A QLabel that emits a clicked() signal on mouse release.
 *
 * Used inside tag and tag-family widgets so that clicking on the text
 * label enters inline-edit mode.
 */
class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    /*! \brief Constructs the label and sets the cursor to a pointing hand.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit ClickableLabel(QWidget* parent = nullptr);

signals:
    /*! \brief Emitted when the label is released with the mouse button.
     *
     * \param event The mouse event from the release.
     */
    void clicked(QMouseEvent *event);

protected:
    /*! \brief Overrides the Qt base-class handler to emit the clicked() signal.
     *
     * \param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

};

#endif // CLICKABLELABEL_H
