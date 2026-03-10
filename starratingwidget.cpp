#include "starratingwidget.h"

#include <cmath>
#include <QEvent>
#include <QPainter>
#include <QMouseEvent>

static const QColor kStarEmpty (200, 200, 200);       // light gray
static const QColor kStarFilled(255, 195,   0);       // gold
static const QColor kStarHover (255, 220,  80);       // lighter gold for hover preview

/*! \brief Constructs a StarRatingWidget with no rating and interactive mode.
 *
 * \param parent Optional Qt parent widget.
 */
StarRatingWidget::StarRatingWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);   // mouseMoveEvent fires without a button held
}

/*! \brief Returns the current rating, or std::nullopt if no rating is set. */
std::optional<int> StarRatingWidget::rating() const
{
    return rating_;
}

/*! \brief Sets the displayed rating without emitting ratingChanged().
 *
 * \param rating A value in [1,5], or std::nullopt to show all stars empty.
 */
void StarRatingWidget::setRating(std::optional<int> rating)
{
    rating_ = rating;
    update();
}

/*! \brief When true, mouse events are ignored and no hover feedback is shown.
 *
 * \param readOnly True to disable interaction.
 */
void StarRatingWidget::setReadOnly(bool readOnly)
{
    readOnly_ = readOnly;
    if (readOnly_) {
        hoverStar_ = 0;
        setMouseTracking(false);
    } else {
        setMouseTracking(true);
    }
    update();
}

/*! \brief Returns true when the widget is in read-only mode. */
bool StarRatingWidget::isReadOnly() const
{
    return readOnly_;
}

/*! \brief Returns the preferred size (120 × 24). */
QSize StarRatingWidget::sizeHint() const
{
    return QSize(120, 24);
}

/*! \brief Returns the minimum usable size (60 × 16). */
QSize StarRatingWidget::minimumSizeHint() const
{
    return QSize(60, 16);
}

/*! \brief Draws the five stars. */
void StarRatingWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!isEnabled())
        painter.setOpacity(0.35);

    const double slotW     = width() / 5.0;
    const double starRadius = qMin(slotW, static_cast<double>(height())) * 0.42;
    const double cy         = height() / 2.0;

    // Determine how many stars should appear filled:
    // hover takes priority over the stored rating while the cursor is inside.
    const int filled = hoverStar_ > 0
                           ? hoverStar_
                           : (rating_.has_value() ? rating_.value() : 0);

    for (int i = 1; i <= 5; ++i) {
        const double cx = slotW * (i - 0.5);
        QPainterPath path = starPath(QPointF(cx, cy), starRadius);

        QColor color;
        if (i <= filled) {
            color = (hoverStar_ > 0) ? kStarHover : kStarFilled;
        } else {
            color = kStarEmpty;
        }
        painter.fillPath(path, color);
    }
}

/*! \brief Updates the hover highlight as the cursor moves over the widget. */
void StarRatingWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (readOnly_) { QWidget::mouseMoveEvent(event); return; }

    const int s = starAtX(static_cast<int>(event->position().x()));
    if (s != hoverStar_) {
        hoverStar_ = s;
        update();
    }
    event->accept();
}

/*! \brief Sets or clears the rating when the user clicks. */
void StarRatingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (readOnly_) { QWidget::mouseReleaseEvent(event); return; }

    if (event->button() == Qt::LeftButton) {
        const int clicked = starAtX(static_cast<int>(event->position().x()));
        // Clicking the already-active star clears the rating
        if (rating_.has_value() && rating_.value() == clicked)
            rating_ = std::nullopt;
        else
            rating_ = clicked;

        emit ratingChanged(rating_);
        update();
    }
    event->accept();
}

/*! \brief Clears hover state when the widget is disabled. */
void StarRatingWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange && !isEnabled()) {
        hoverStar_ = 0;
        update();
    }
    QWidget::changeEvent(event);
}

/*! \brief Clears the hover highlight when the cursor leaves the widget. */
void StarRatingWidget::leaveEvent(QEvent *event)
{
    hoverStar_ = 0;
    update();
    QWidget::leaveEvent(event);
}

/*! \brief Returns the star index (1–5) under x-coordinate \p x. */
int StarRatingWidget::starAtX(int x) const
{
    const double slotW = width() / 5.0;
    return qBound(1, static_cast<int>(x / slotW) + 1, 5);
}

/*! \brief Builds a five-pointed star path centred at \p centre with outer radius \p r.
 *
 * The star points upward.  Inner vertices are placed at radius r × 0.382
 * (the reciprocal of the golden ratio), which gives the classic star shape.
 */
QPainterPath StarRatingWidget::starPath(QPointF centre, double r)
{
    constexpr int    kPoints      = 5;
    constexpr double kInnerRatio  = 0.382;
    constexpr double kStartAngle  = -M_PI / 2.0;   // first point straight up
    constexpr double kStep        = M_PI / kPoints; // 36° between outer and inner

    const double ri = r * kInnerRatio;

    QPainterPath path;
    for (int i = 0; i < kPoints * 2; ++i) {
        const double angle  = kStartAngle + i * kStep;
        const double radius = (i % 2 == 0) ? r : ri;
        const QPointF pt(centre.x() + radius * std::cos(angle),
                         centre.y() + radius * std::sin(angle));
        if (i == 0) path.moveTo(pt);
        else        path.lineTo(pt);
    }
    path.closeSubpath();
    return path;
}
