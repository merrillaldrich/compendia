#ifndef GEO_H
#define GEO_H

#include <functional>
#include <optional>

#include <QMap>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QString>

/*! \brief Static utility class for GPS coordinate parsing, OSM tile math,
 *         and Nominatim reverse geocoding.
 *
 * All methods are static; no instance is needed.
 */
class Geo
{
public:
    /*! \brief Parses GPS coordinates from a libexif EXIF map.
     *
     * Reads "GPSLatitude", "GPSLatitudeRef", "GPSLongitude", and
     * "GPSLongitudeRef" from \a exifMap.  Accepts both DMS rational format
     * ("48/1, 8/1, 2850/100") and plain decimal ("48.141250").
     * South latitudes and West longitudes are returned as negative values.
     *
     * \param exifMap EXIF key-value map produced by ExifParser::getExifMap().
     * \return QPointF(latitude, longitude) in decimal degrees, or nullopt on failure.
     */
    static std::optional<QPointF> parseGpsCoordinates(const QMap<QString, QString> &exifMap);

    /*! \brief Returns the OSM tile indices for a coordinate at the given zoom level.
     *
     * \param lat  Latitude in decimal degrees.
     * \param lon  Longitude in decimal degrees.
     * \param zoom Zoom level (0–19).
     * \return Tile column (x) and row (y).
     */
    static QPoint latLonToTile(double lat, double lon, int zoom);

    /*! \brief Returns the world-pixel position of a coordinate at the given zoom level.
     *
     * The world-pixel origin is the top-left corner of the tile grid. Each tile
     * is 256 × 256 px. The result can be used to compute tile offsets within
     * a widget viewport.
     *
     * \param lat  Latitude in decimal degrees.
     * \param lon  Longitude in decimal degrees.
     * \param zoom Zoom level (0–19).
     * \return World-pixel position (x = horizontal, y = vertical).
     */
    static QPointF latLonToPixel(double lat, double lon, int zoom);

    /*! \brief Converts a world-pixel position back to decimal-degree coordinates.
     *
     * \param px   World-pixel x.
     * \param py   World-pixel y.
     * \param zoom Zoom level (0–19).
     * \return {latitude, longitude} pair.
     */
    static std::pair<double, double> pixelToLatLon(double px, double py, int zoom);

    /*! \brief Builds the OpenStreetMap tile URL for tile (x, y) at \a zoom.
     *
     * \param x    Tile column.
     * \param y    Tile row.
     * \param zoom Zoom level.
     * \return URL string ready for a network request.
     */
    static QString osmTileUrl(int x, int y, int zoom);

    /*! \brief Initiates an asynchronous Nominatim reverse-geocode request.
     *
     * The HTTP request is sent via MapTileCache::instance(). \a callback is
     * invoked on the main thread with (city, state, country) strings when the
     * response arrives.  If the lookup fails all three strings are empty.
     * The callback is silently dropped if \a context is destroyed before
     * the reply arrives.
     *
     * \param lat      Latitude in decimal degrees.
     * \param lon      Longitude in decimal degrees.
     * \param context  Object whose lifetime guards the callback.
     * \param callback Invoked with (city, state, country) on success or ("","","") on failure.
     */
    static void reverseGeocode(double lat, double lon,
                               QObject *context,
                               std::function<void(QString city,
                                                  QString state,
                                                  QString country)> callback);

private:
    static double parseRational(const QString &token);
    static double dmsToDecimal(const QString &dms);
};

#endif // GEO_H
