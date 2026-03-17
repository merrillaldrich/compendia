#ifndef WRAPLABEL_H
#define WRAPLABEL_H

#include <QLabel>
#include <QResizeEvent>

/*! \brief A QLabel that grows in height to show all word-wrapped text.
 *
 * The core problem with QLabel in a QFormLayout is that sizeHint() is queried
 * before the layout assigns the widget its final width, so heightForWidth()
 * always uses a stale width and the result is always one pass behind.
 *
 * The fix: override setText() — which is called when the actual content is
 * known and the widget already has its stable column width — and immediately
 * set the minimum height via setMinimumHeight(heightForWidth(width())).  The
 * form layout's minimum-size accounting picks this up, the scroll area resizes
 * the content widget, and the label shows its full text in a single pass.
 *
 * resizeEvent() repeats the update if the column width ever changes (e.g. when
 * the panel is resized).
 */
class WrapLabel : public QLabel
{
public:
    /*! \brief Constructs a WrapLabel with word-wrap and Minimum vertical size policy enabled.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit WrapLabel(QWidget *parent = nullptr) : QLabel(parent)
    {
        setWordWrap(true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }

    /*! \brief Sets the label text and immediately updates the minimum height.
     *
     * Not virtual in QLabel, but the generated UI code holds WrapLabel* so
     * this override is called whenever MainWindow sets property values.
     */
    void setText(const QString &text)
    {
        QLabel::setText(text);
        applyMinimumHeight();
    }

    /*! \brief Returns true to advertise that this label implements heightForWidth(). */
    bool hasHeightForWidth() const override { return true; }

    /*! \brief Computes the pixel height required to display all text at width \a w.
     *
     * \param w Available pixel width.
     * \return Required height in pixels.
     */
    int heightForWidth(int w) const override
    {
        if (w <= 0) w = QLabel::sizeHint().width();
        const QMargins m = contentsMargins();
        const int innerW = w - m.left() - m.right();
        const int innerH = fontMetrics()
                               .boundingRect(QRect(0, 0, innerW, INT_MAX),
                                             Qt::AlignLeft | Qt::TextWordWrap,
                                             text())
                               .height();
        return innerH + m.top() + m.bottom();
    }

    /*! \brief Returns a zero minimum size so the label never forces the layout wider than needed. */
    QSize minimumSizeHint() const override
    {
        return {0, 0};
    }

    /*! \brief Returns a size whose height is computed from the current column width. */
    QSize sizeHint() const override
    {
        int w = width() > 0 ? width() : QLabel::sizeHint().width();
        return {QLabel::sizeHint().width(), heightForWidth(w)};
    }

protected:
    /*! \brief Re-applies the minimum height when the column width changes.
     *
     * \param e The resize event.
     */
    void resizeEvent(QResizeEvent *e) override
    {
        QLabel::resizeEvent(e);
        // Keep minimum height in sync when the column width changes.
        if (e->size().width() != e->oldSize().width())
            applyMinimumHeight();
    }

private:
    /*! \brief Sets the widget's minimum height to the value returned by heightForWidth(width()). */
    void applyMinimumHeight()
    {
        if (width() > 0)
            setMinimumHeight(heightForWidth(width()));
    }
};

#endif // WRAPLABEL_H
