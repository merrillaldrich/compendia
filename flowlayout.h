#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QStyle>

/*! \brief A custom QLayout that arranges child widgets in a wrapping flow, like inline text.
 *
 * Items are placed left-to-right and wrap to the next row when the available
 * width is exhausted.  The layout supports both a parent-widget form and a
 * standalone form, and exposes independent horizontal and vertical spacing.
 */
class FlowLayout : public QLayout
{
public:
    /*! \brief Constructs a FlowLayout parented to a widget with optional spacing.
     *
     * \param parent    The parent widget that owns this layout.
     * \param margin    Uniform content margin (pass -1 to use the style default).
     * \param hSpacing  Horizontal spacing between items (pass -1 for style default).
     * \param vSpacing  Vertical spacing between rows (pass -1 for style default).
     */
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);

    /*! \brief Constructs a standalone FlowLayout with optional spacing.
     *
     * \param margin    Uniform content margin (pass -1 to use the style default).
     * \param hSpacing  Horizontal spacing between items (pass -1 for style default).
     * \param vSpacing  Vertical spacing between rows (pass -1 for style default).
     */
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);

    /*! \brief Destroys the layout and deletes all managed layout items. */
    ~FlowLayout();

    /*! \brief Appends an item to the internal item list.
     *
     * \param item The layout item to add.
     */
    void addItem(QLayoutItem *item) override;

    /*! \brief Returns the horizontal spacing between items.
     *
     * \return The horizontal spacing in pixels.
     */
    int horizontalSpacing() const;

    /*! \brief Returns the vertical spacing between rows.
     *
     * \return The vertical spacing in pixels.
     */
    int verticalSpacing() const;

    /*! \brief Overrides the Qt base-class method; flow layout does not expand in any direction.
     *
     * \return An empty orientations flags value.
     */
    Qt::Orientations expandingDirections() const override;

    /*! \brief Overrides the Qt base-class method to indicate this layout has a height-for-width policy.
     *
     * \return Always returns true.
     */
    bool hasHeightForWidth() const override;

    /*! \brief Overrides the Qt base-class method to compute the required height for a given width.
     *
     * \param width The available width in pixels.
     * \return The minimum height required to lay out all items at that width.
     */
    int heightForWidth(int) const override;

    /*! \brief Returns the number of items managed by this layout.
     *
     * \return The item count.
     */
    int count() const override;

    /*! \brief Returns the layout item at the given index without removing it.
     *
     * \param index Zero-based index of the item.
     * \return Pointer to the item, or nullptr if the index is out of range.
     */
    QLayoutItem *itemAt(int index) const override;

    /*! \brief Returns the minimum size needed to display at least one item with margins.
     *
     * \return The minimum bounding size.
     */
    QSize minimumSize() const override;

    /*! \brief Overrides the Qt base-class method to position all items within the given rect.
     *
     * \param rect The rectangle within which to arrange items.
     */
    void setGeometry(const QRect &rect) override;

    /*! \brief Returns the preferred size, equal to the minimum size.
     *
     * \return The size hint.
     */
    QSize sizeHint() const override;

    /*! \brief Removes and returns the layout item at the given index.
     *
     * \param index Zero-based index of the item to remove.
     * \return Pointer to the removed item, or nullptr if out of range.
     */
    QLayoutItem *takeAt(int index) override;

private:
    /*! \brief Performs the actual flow layout calculation, optionally applying geometry.
     *
     * \param rect     The rectangle to lay out within.
     * \param testOnly If true, only calculates the required height without moving items.
     * \return The total height required by the laid-out items.
     */
    int doLayout(const QRect &rect, bool testOnly) const;

    /*! \brief Returns the appropriate spacing from the parent widget's style if no explicit value was set.
     *
     * \param pm The pixel-metric to query from the style.
     * \return The spacing in pixels, or -1 if no parent is available.
     */
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList; ///< Ordered list of managed layout items.
    int m_hSpace; ///< Explicit horizontal spacing between items, or -1 for style default.
    int m_vSpace; ///< Explicit vertical spacing between rows, or -1 for style default.
};

#endif // FLOWLAYOUT_H
