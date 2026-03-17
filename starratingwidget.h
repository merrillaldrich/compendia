#ifndef STARRATINGWIDGET_H
#define STARRATINGWIDGET_H

#include <optional>
#include <QWidget>
#include <QPainterPath>

/*! \brief A five-star rating widget that can display, set, or clear a 1–5 rating.
 *
 * Stars are drawn gray when empty and gold when filled.  Hovering previews the
 * rating that would be set on click.  Clicking the already-active star clears
 * the rating back to "no rating" (std::nullopt).
 *
 * The widget emits ratingChanged() only on user interaction; calling setRating()
 * programmatically is silent.  Set read-only mode via setReadOnly() to suppress
 * all mouse interaction (useful for display-only contexts).
 *
 * Typical uses:
 *   - Inline editor on the file-properties panel (interactive, bound to a TaggedFile)
 *   - Filter control in the filter bar (interactive, drives FilterProxyModel)
 *   - Thumbnail overlay or list-column indicator (read-only)
 */
class StarRatingWidget : public QWidget
{
    Q_OBJECT

public:
    /*! \brief Constructs a StarRatingWidget with no rating and interactive mode.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit StarRatingWidget(QWidget *parent = nullptr);

    /*! \brief Returns the current rating, or std::nullopt if no rating is set. */
    std::optional<int> rating() const;

    /*! \brief Sets the displayed rating without emitting ratingChanged().
     *
     * \param rating A value in [1,5], or std::nullopt to show all stars empty.
     */
    void setRating(std::optional<int> rating);

    /*! \brief When true, mouse events are ignored and no hover feedback is shown.
     *
     * \param readOnly True to disable interaction.
     */
    void setReadOnly(bool readOnly);

    /*! \brief Returns true when the widget is in read-only mode. */
    bool isReadOnly() const;

    /*! \brief Returns the preferred size (120 × 24). */
    QSize sizeHint() const override;

    /*! \brief Returns the minimum usable size (60 × 16). */
    QSize minimumSizeHint() const override;

signals:
    /*! \brief Emitted when the user clicks a star.
     *
     * \param rating The new rating [1,5], or std::nullopt when the rating was cleared.
     */
    void ratingChanged(std::optional<int> rating);

protected:
    /*! \brief Draws the five stars. */
    void paintEvent(QPaintEvent *event) override;

    /*! \brief Updates the hover highlight as the cursor moves over the widget. */
    void mouseMoveEvent(QMouseEvent *event) override;

    /*! \brief Sets or clears the rating when the user clicks. */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /*! \brief Clears the hover highlight when the cursor leaves the widget. */
    void leaveEvent(QEvent *event) override;

    /*! \brief Clears hover state when the widget is disabled. */
    void changeEvent(QEvent *event) override;

private:
    std::optional<int> rating_ = std::nullopt; ///< Current rating [1,5], or std::nullopt when unrated.
    int    hoverStar_ = 0;   ///< 1–5 while hovering, 0 when not hovering
    bool   readOnly_  = false; ///< When true, mouse events are ignored.

    /*! \brief Returns the star index (1–5) under x-coordinate \p x. */
    int starAtX(int x) const;

    /*! \brief Builds a five-pointed star path centred at \p centre with outer radius \p r. */
    static QPainterPath starPath(QPointF centre, double r);
};

#endif // STARRATINGWIDGET_H
