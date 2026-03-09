#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPainter>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>
#include <QApplication>
#include <QLayout>
#include "tag.h"
#include "clickablelabel.h"
#include "variablewidthlineedit.h"
#include "tagwidgetclosebutton.h"
#include "mainwindow.h"
#include "taggingwidget.h"

/*! \brief Visual widget representing a single tag, supporting inline editing and drag-and-drop.
 *
 * Displays a rounded-rectangle pill containing a close button, and a ClickableLabel
 * that switches to a VariableWidthLineEdit for inline name editing.  The widget can
 * be dragged onto TaggableListView or container targets to apply the tag.
 */
class TagWidget : public TaggingWidget
{
    Q_OBJECT

private:
    QString edit_status_ = "Read";
    VariableWidthLineEdit* line_edit_;
    ClickableLabel* label_;
    QPoint drag_start_position_;
    bool in_library_ = false;
    Tag* tag_;

public:
    /*! \brief Constructs a default, empty TagWidget.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TagWidget(QWidget *parent = nullptr);

    /*! \brief Constructs a TagWidget bound to the given Tag and styled with its family colour.
     *
     * \param tag    The Tag this widget represents.
     * \param parent Qt parent widget.
     */
    TagWidget(Tag *tag, QWidget *parent);

    /*! \brief Switches the widget into inline-edit mode, showing the line edit. */
    void startEdit();

    /*! \brief Commits the edited name to the Tag and switches back to read mode. */
    void endEdit();

    /*! \brief Returns the Tag pointer this widget represents.
     *
     * \return The associated Tag.
     */
    Tag* getTag();

    /*! \brief Returns the preferred size of the widget.
     *
     * \return The size hint.
     */
    QSize sizeHint() const override;

    /*! \brief Returns the minimum size of the widget.
     *
     * \return The minimum size hint.
     */
    QSize minimumSizeHint() const override;

protected:
    /*! \brief Overrides the Qt base-class paint handler to draw the coloured rounded-rectangle background.
     *
     * \param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /*! \brief Overrides the Qt base-class mouse-press handler to record the drag start position.
     *
     * \param event The mouse press event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /*! \brief Overrides the Qt base-class mouse-move handler to initiate a drag operation.
     *
     * \param event The mouse move event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /*! \brief Recalculates and applies the widget width to fit the current label or line-edit text. */
    void adjustCustomWidth();

private slots:
    /*! \brief Slot called when the line edit finishes editing; commits the change via endEdit(). */
    void onLineEditEditingFinished();

    /*! \brief Slot called when the label is clicked; enters inline-edit mode via startEdit().
     *
     * \param event The mouse event from the click.
     */
    void onLabelClicked(QMouseEvent *event);

    /*! \brief Slot called when the underlying Tag's name changes; updates the label text. */
    void onTagNameChanged();

    /*! \brief Slot called as the user types in the line edit; adjusts the widget width. */
    void onTextEdited();

    /*! \brief Slot called when the close button is clicked; emits deleteRequested(). */
    void onCloseButtonClicked();

signals:
    /*! \brief Emitted when the user clicks the close button to remove the tag.
     *
     * \param t The Tag to be removed.
     */
    void deleteRequested(Tag *t);

    /*! \brief Emitted after the tag name has been successfully changed.
     *
     * \param t The Tag whose name changed.
     */
    void tagNameChanged(Tag *t);

    /*! \brief Emitted when a newly created tag is abandoned without entering a name.
     *
     * \param tw This TagWidget, so the parent can clean it up.
     */
    void abandonRequested(TagWidget *tw);
};

#endif // TAGWIDGET_H
