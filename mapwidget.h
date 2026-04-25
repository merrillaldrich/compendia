#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QVector>
#include <QWidget>

/*! \brief Tile-based OpenStreetMap widget with an optional red marker at the photo location.
 *
 * In \e interactive mode the user can drag to pan and use the scroll wheel to zoom.
 * In non-interactive (overlay) mode a click emits clicked() so the caller can open
 * a larger map dialog.  Both modes share the same MapTileCache singleton.
 *
 * The widget paints with rounded corners so it blends naturally over the image preview.
 */
class MapWidget : public QWidget
{
    Q_OBJECT

public:
    /*! \brief Geographic bounding box of the current viewport in decimal degrees. */
    struct ViewportBounds {
        double minLat; ///< Southern edge.
        double maxLat; ///< Northern edge.
        double minLon; ///< Western edge.
        double maxLon; ///< Eastern edge.
    };

    /*! \brief Constructs a MapWidget.
     *
     * \param interactive True to enable pan and zoom; false for display-only overlay.
     * \param parent      Optional Qt parent widget.
     */
    explicit MapWidget(bool interactive = false, QWidget *parent = nullptr);

    /*! \brief Centers the map at (lat, lon) and redraws. */
    void setLocation(double lat, double lon);

    /*! \brief Sets the OSM zoom level (clamped to [1, 19]) and redraws. */
    void setZoomLevel(int z);

    /*! \brief Sets the collection of photo locations rendered as blue dots.
     *
     * Each QPointF stores (latitude, longitude) — same convention as
     * Geo::parseGpsCoordinates().  When non-empty the red single-photo marker
     * is suppressed and the dots layer is drawn instead.
     *
     * \param points Vector of (lat, lon) points.
     */
    void setGeoPoints(const QVector<QPointF> &points);

    /*! \brief Returns the geographic bounding box of the current viewport.
     *
     * Computed from the current centre lat/lon, zoom level, and widget dimensions.
     * Used by GeoFilterDialog to determine which photos are in view.
     */
    ViewportBounds viewportBounds() const;

signals:
    /*! \brief Emitted when the user clicks the widget in non-interactive mode. */
    void clicked();

    /*! \brief Emitted after any pan or zoom changes the visible viewport.
     *
     * Connect to this signal to update UI elements (e.g. a status label showing
     * how many photo dots are currently visible) without polling.
     */
    void viewportChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onTileReady(int x, int y, int zoom, QPixmap pix);

private:
    void drawTiles(QPainter &p);
    void drawMarker(QPainter &p);
    void drawGeoPoints(QPainter &p);

    double lat_          = 0.0;   ///< Current viewport centre latitude.
    double lon_          = 0.0;   ///< Current viewport centre longitude.
    double photoLat_     = 0.0;   ///< Original photo location latitude (fixed; marker follows this).
    double photoLon_     = 0.0;   ///< Original photo location longitude.
    int    zoom_         = 13;
    bool   interactive_;
    bool   locationSet_  = false;

    bool   panning_      = false;
    QPoint panStart_;
    double panOriginLat_ = 0.0;
    double panOriginLon_ = 0.0;

    QVector<QPointF> geoPoints_; ///< Multi-point overlay (lat, lon); suppresses single marker when non-empty.
};

#endif // MAPWIDGET_H
