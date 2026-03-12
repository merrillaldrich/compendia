// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "flowlayout.h"

/*! \brief Constructs a FlowLayout parented to a widget with optional spacing.
 *
 * \param parent    The parent widget that owns this layout.
 * \param margin    Uniform content margin (pass -1 to use the style default).
 * \param hSpacing  Horizontal spacing between items (pass -1 for style default).
 * \param vSpacing  Vertical spacing between rows (pass -1 for style default).
 */
FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

/*! \brief Constructs a standalone FlowLayout with optional spacing.
 *
 * \param margin    Uniform content margin (pass -1 to use the style default).
 * \param hSpacing  Horizontal spacing between items (pass -1 for style default).
 * \param vSpacing  Vertical spacing between rows (pass -1 for style default).
 */
FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

/*! \brief Destroys the layout and deletes all managed layout items. */
FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

/*! \brief Appends an item to the internal item list.
 *
 * \param item The layout item to add.
 */
void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

/*! \brief Returns the horizontal spacing between items.
 *
 * \return The horizontal spacing in pixels.
 */
int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

/*! \brief Returns the vertical spacing between rows.
 *
 * \return The vertical spacing in pixels.
 */
int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

/*! \brief Returns the number of items managed by this layout.
 *
 * \return The item count.
 */
int FlowLayout::count() const
{
    return itemList.size();
}

/*! \brief Returns the layout item at the given index without removing it.
 *
 * \param index Zero-based index of the item.
 * \return Pointer to the item, or nullptr if the index is out of range.
 */
QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

/*! \brief Removes and returns the layout item at the given index.
 *
 * \param index Zero-based index of the item to remove.
 * \return Pointer to the removed item, or nullptr if out of range.
 */
QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    return nullptr;
}

/*! \brief Overrides the Qt base-class method; flow layout does not expand in any direction.
 *
 * \return An empty orientations flags value.
 */
Qt::Orientations FlowLayout::expandingDirections() const
{
    return { };
}

/*! \brief Overrides the Qt base-class method to indicate this layout has a height-for-width policy.
 *
 * \return Always returns true.
 */
bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

/*! \brief Overrides the Qt base-class method to compute the required height for a given width.
 *
 * \param width The available width in pixels.
 * \return The minimum height required to lay out all items at that width.
 */
int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

/*! \brief Overrides the Qt base-class method to position all items within the given rect.
 *
 * \param rect The rectangle within which to arrange items.
 */
void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

/*! \brief Returns the preferred size, equal to the minimum size.
 *
 * \return The size hint.
 */
QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

/*! \brief Returns the minimum size needed to display at least one item with margins.
 *
 * \return The minimum bounding size.
 */
QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem *item : std::as_const(itemList))
        size = size.expandedTo(item->minimumSize());

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

/*! \brief Performs the actual flow layout calculation, optionally applying geometry.
 *
 * \param rect     The rectangle to lay out within.
 * \param testOnly If true, only calculates the required height without moving items.
 * \return The total height required by the laid-out items.
 */
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    for (QLayoutItem *item : std::as_const(itemList)) {
        const QWidget *wid = item->widget();
        int spaceX = horizontalSpacing();
        if (spaceX == -1)
            spaceX = wid->style()->layoutSpacing(
                QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        int spaceY = verticalSpacing();
        if (spaceY == -1)
            spaceY = wid->style()->layoutSpacing(
                QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

        int nextX = x + item->sizeHint().width() + spaceX;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + item->sizeHint().width() + spaceX;
            lineHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    return y + lineHeight - rect.y() + bottom;
}

/*! \brief Returns the appropriate spacing from the parent widget's style if no explicit value was set.
 *
 * \param pm The pixel-metric to query from the style.
 * \return The spacing in pixels, or -1 if no parent is available.
 */
int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
