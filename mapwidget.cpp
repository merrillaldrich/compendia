#include "mapwidget.h"
#include "geo.h"
#include "maptilecache.h"
#include "constants.h"

#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>

static constexpr int    kTileSize    = 256;
static constexpr int    kCornerRadius = 8;
static constexpr double kLatMax      = 85.051129;

MapWidget::MapWidget(bool interactive, QWidget *parent)
    : QWidget(parent)
    , interactive_(interactive)
{
    if (interactive)
        setCursor(Qt::OpenHandCursor);

    connect(MapTileCache::instance(), &MapTileCache::tileReady,
            this, &MapWidget::onTileReady);
}

void MapWidget::setLocation(double lat, double lon)
{
    lat_         = qBound(-kLatMax, lat, kLatMax);
    lon_         = qBound(-180.0, lon, 180.0);
    photoLat_    = lat_;
    photoLon_    = lon_;
    locationSet_ = true;
    update();
}

void MapWidget::setZoomLevel(int z)
{
    zoom_ = qBound(1, z, 19);
    update();
}

void MapWidget::onTileReady(int /*x*/, int /*y*/, int zoom, QPixmap /*pix*/)
{
    if (zoom == zoom_)
        update();
}

// ---------------------------------------------------------------------------
// Paint helpers
// ---------------------------------------------------------------------------

void MapWidget::drawTiles(QPainter &p)
{
    const QPointF centerPx = Geo::latLonToPixel(lat_, lon_, zoom_);
    const double  originX  = centerPx.x() - width()  / 2.0;
    const double  originY  = centerPx.y() - height() / 2.0;

    const int maxTile   = (1 << zoom_) - 1;
    const int firstTileX = static_cast<int>(std::floor(originX / kTileSize));
    const int firstTileY = static_cast<int>(std::floor(originY / kTileSize));
    const int lastTileX  = static_cast<int>(std::floor((originX + width())  / kTileSize));
    const int lastTileY  = static_cast<int>(std::floor((originY + height()) / kTileSize));

    for (int ty = firstTileY; ty <= lastTileY; ++ty) {
        for (int tx = firstTileX; tx <= lastTileX; ++tx) {
            const int cx = qBound(0, tx, maxTile);
            const int cy = qBound(0, ty, maxTile);

            const int drawX = static_cast<int>(tx * kTileSize - originX);
            const int drawY = static_cast<int>(ty * kTileSize - originY);

            const QPixmap pix = MapTileCache::instance()->tilePixmap(cx, cy, zoom_);
            if (!pix.isNull())
                p.drawPixmap(drawX, drawY, pix);
        }
    }
}

void MapWidget::drawMarker(QPainter &p)
{
    const QPointF viewCenterPx = Geo::latLonToPixel(lat_,      lon_,      zoom_);
    const QPointF photoPx      = Geo::latLonToPixel(photoLat_, photoLon_, zoom_);
    const int cx = static_cast<int>(width()  / 2.0 + (photoPx.x() - viewCenterPx.x()));
    const int cy = static_cast<int>(height() / 2.0 + (photoPx.y() - viewCenterPx.y()));

    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    // Drop shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawEllipse(cx - 6, cy - 5, 13, 13);

    // Red filled circle with white outline
    p.setBrush(QColor(220, 50, 50));
    p.setPen(QPen(Qt::white, 1.5));
    p.drawEllipse(cx - 6, cy - 6, 12, 12);

    p.restore();
}

// ---------------------------------------------------------------------------
// QWidget overrides
// ---------------------------------------------------------------------------

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Clip to rounded rectangle
    QPainterPath clip;
    clip.addRoundedRect(rect(), kCornerRadius, kCornerRadius);
    p.setClipPath(clip);

    // Light grey background (visible while tiles are loading)
    p.fillRect(rect(), QColor(210, 210, 210));

    if (locationSet_) {
        drawTiles(p);
        drawMarker(p);
    }

    // Rounded border (drawn outside the clip so it sits on top)
    p.setClipping(false);
    p.setPen(QPen(QColor(90, 90, 90, 160), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), kCornerRadius, kCornerRadius);
}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (interactive_ && event->button() == Qt::LeftButton) {
        panning_      = true;
        panStart_     = event->pos();
        panOriginLat_ = lat_;
        panOriginLon_ = lon_;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!interactive_ || !panning_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF originPx = Geo::latLonToPixel(panOriginLat_, panOriginLon_, zoom_);
    const QPointF delta    = event->pos() - panStart_;
    const QPointF newPx    = originPx - delta;

    auto [newLat, newLon] = Geo::pixelToLatLon(newPx.x(), newPx.y(), zoom_);
    lat_ = qBound(-kLatMax, newLat, kLatMax);
    lon_ = qBound(-180.0,   newLon, 180.0);
    update();
    event->accept();
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (interactive_) {
        if (panning_) {
            panning_ = false;
            setCursor(Qt::OpenHandCursor);
            event->accept();
            return;
        }
    } else {
        // Non-interactive: a release that started as a press here = a click
        if (event->button() == Qt::LeftButton) {
            emit clicked();
            event->accept();
            return;
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    if (!interactive_) {
        event->ignore();
        return;
    }
    if (event->angleDelta().y() > 0)
        setZoomLevel(zoom_ + 1);
    else
        setZoomLevel(zoom_ - 1);
    event->accept();
}
