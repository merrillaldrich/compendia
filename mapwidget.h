#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QPixmap>
#include <QPoint>
#include <QPointF>
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

signals:
    /*! \brief Emitted when the user clicks the widget in non-interactive mode. */
    void clicked();

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
};

#endif // MAPWIDGET_H
