#ifndef TAGFAMILYWIDGET_H
#define TAGFAMILYWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "flowlayout.h"
#include "tagfamily.h"
#include "tagwidget.h"
#include "clickablelabel.h"
#include "variablewidthlineedit.h"
#include "mainwindow.h"
#include "taggingwidget.h"
#include "tagfamilywidgetcollapsebutton.h"

/*! \brief Visual widget representing a tag family, containing its child TagWidget items.
 *
 * Draws a rounded-rectangle outline with a filled label area at the top showing
 * the family name.  Clicking the body (below the label) adds a new Tag; clicking
 * the label enters inline-edit mode for the family name.  Child tags are laid out
 * using a FlowLayout.
 */
class TagFamilyWidget : public TaggingWidget
{
    Q_OBJECT

private:
    QString edit_status_ = "Read";
    VariableWidthLineEdit* line_edit_;
    ClickableLabel* label_;
    bool in_library_ = false;
    TagFamily* tag_family_;
    bool collapsed_ = false;
    TagFamilyWidgetCollapseButton *collapseButton_ = nullptr;

protected:
    /*! \brief Overrides the Qt base-class mouse-release handler to add a new tag on click.
     *
     * \param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event);

    /*! \brief Overrides the Qt base-class resize handler to keep the collapse button anchored.
     *
     * \param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /*! \brief Returns the minimum size as the size hint.
     *
     * \return The size hint.
     */
    QSize sizeHint() const ;

public:
    /*! \brief Constructs a default, empty TagFamilyWidget.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TagFamilyWidget(QWidget *parent = nullptr);

    /*! \brief Constructs a TagFamilyWidget bound to the given TagFamily.
     *
     * \param tagFamily The TagFamily this widget represents.
     * \param parent    Qt parent widget.
     */
    TagFamilyWidget(TagFamily *tagFamily, QWidget *parent);

    /*! \brief Overrides the Qt base-class paint handler to draw the family border and label area.
     *
     * \param event The paint event.
     */
    void paintEvent(QPaintEvent *event);

    /*! \brief Switches the widget into inline-edit mode for the family name. */
    void startEdit();

    /*! \brief Commits the edited family name and switches back to read mode. */
    void endEdit();

    /*! \brief Returns the TagFamily pointer this widget represents.
     *
     * \return The associated TagFamily.
     */
    TagFamily* getTagFamily() const;

    /*! \brief Sorts the child TagWidget items alphabetically by tag name. */
    void sort();

public slots:
    /*! \brief Called when a child TagWidget's width changes while it is being edited.
     *
     * Re-measures the layout height at the current width and adjusts this widget's
     * minimum height if a row was added or removed, without posting a LayoutRequest
     * to the event queue from inside setGeometry.
     */
    void onChildTagWidthChanged();

    /*! \brief Recalculates and applies the correct minimum height for the current set
     *  of child tag widgets.
     *
     * Call this after adding or removing child TagWidget items programmatically so
     * the family widget expands or contracts to fit.
     */
    void refreshMinimumHeight();

private slots:
    /*! \brief Slot called when the line edit finishes editing; commits via endEdit(). */
    void onLineEditEditingFinished();

    /*! \brief Slot called when the collapse button is clicked; toggles the collapsed state. */
    void toggleCollapsed();

    /*! \brief Creates a new empty TagWidget, wires its signals, adds it to the layout, and
     *  puts it into edit mode.  Used by both mouseReleaseEvent and endEdit(). */
    void createAndEditNewTag();

    /*! \brief Slot called when the family name label is clicked; enters edit mode via startEdit().
     *
     * \param event The mouse event from the click.
     */
    void onLabelClicked(QMouseEvent *event);

    /*! \brief Slot called when the underlying TagFamily name changes; refreshes the label. */
    void onTagFamilyNameChanged();

    /*! \brief Slot called when a child tag name changes; re-sorts the tag widgets. */
    void onTagNameChanged();

signals:

};

#endif // TAGFAMILYWIDGET_H
