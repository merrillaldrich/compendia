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

    bool hasHeightForWidth() const override { return true; }

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

    QSize minimumSizeHint() const override
    {
        return {0, 0};
    }

    QSize sizeHint() const override
    {
        int w = width() > 0 ? width() : QLabel::sizeHint().width();
        return {QLabel::sizeHint().width(), heightForWidth(w)};
    }

protected:
    void resizeEvent(QResizeEvent *e) override
    {
        QLabel::resizeEvent(e);
        // Keep minimum height in sync when the column width changes.
        if (e->size().width() != e->oldSize().width())
            applyMinimumHeight();
    }

private:
    void applyMinimumHeight()
    {
        if (width() > 0)
            setMinimumHeight(heightForWidth(width()));
    }
};

#endif // WRAPLABEL_H
