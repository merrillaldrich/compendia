#ifndef MAPTILECACHE_H
#define MAPTILECACHE_H

#include <functional>

#include <QCache>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QString>

/*! \brief Application-wide singleton that fetches and caches OpenStreetMap tile images.
 *
 * All tile requests and Nominatim reverse-geocode requests share a single
 * QNetworkAccessManager.  Up to 200 tiles are kept in an in-memory LRU cache.
 * The singleton must only be accessed from the main thread.
 */
class MapTileCache : public QObject
{
    Q_OBJECT

public:
    /*! \brief Returns the application-wide MapTileCache instance. */
    static MapTileCache *instance();

    /*! \brief Returns the cached pixmap for tile (x, y, zoom), or a null QPixmap.
     *
     * If the tile is not in the cache a background HTTP GET is initiated.
     * When the response arrives \c tileReady() is emitted and callers should
     * call this method again (or repaint) to retrieve the newly cached pixmap.
     * Duplicate in-flight requests are suppressed.
     *
     * \param x    Tile column.
     * \param y    Tile row.
     * \param zoom Zoom level.
     * \return Cached tile pixmap, or a null QPixmap when not yet available.
     */
    QPixmap tilePixmap(int x, int y, int zoom);

    /*! \brief Sends a Nominatim reverse-geocode request for (lat, lon).
     *
     * \param lat      Latitude in decimal degrees.
     * \param lon      Longitude in decimal degrees.
     * \param context  Object whose lifetime guards the callback.  The callback
     *                 is silently dropped if this object is destroyed first.
     * \param callback Invoked on the main thread with (city, state, country).
     *                 All strings are empty on network or parse failure.
     */
    void requestReverseGeocode(double lat, double lon,
                               QObject *context,
                               std::function<void(QString city,
                                                  QString state,
                                                  QString country)> callback);

signals:
    /*! \brief Emitted when a tile has been fetched from the network and added to cache.
     *
     * \param x    Tile column.
     * \param y    Tile row.
     * \param zoom Zoom level.
     * \param pixmap The tile image.
     */
    void tileReady(int x, int y, int zoom, QPixmap pixmap);

private:
    explicit MapTileCache(QObject *parent = nullptr);

    static QString cacheKey(int x, int y, int zoom);

    QNetworkAccessManager   *nam_;
    QCache<QString, QPixmap> tileCache_{200};
    QSet<QString>            inFlight_;
};

#endif // MAPTILECACHE_H
