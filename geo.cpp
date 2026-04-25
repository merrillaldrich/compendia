#include "geo.h"
#include "maptilecache.h"
#include "constants.h"

#include <cmath>

#include <QStringList>

static constexpr double kPi       = M_PI;
static constexpr int    kTileSize = 256;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

double Geo::parseRational(const QString &token)
{
    const QString t = token.trimmed();
    const int slash = t.indexOf('/');
    if (slash < 0)
        return t.toDouble();
    const double num = t.left(slash).toDouble();
    const double den = t.mid(slash + 1).toDouble();
    return (den == 0.0) ? 0.0 : num / den;
}

// Accepts libexif DMS strings such as "48/1, 8/1, 2850/100" or "48, 8, 28".
// Returns NaN on parse failure.
double Geo::dmsToDecimal(const QString &dms)
{
    const QStringList parts = dms.split(',');
    if (parts.size() == 1) {
        bool ok = false;
        const double v = parts[0].trimmed().toDouble(&ok);
        return ok ? v : qQNaN();
    }
    if (parts.size() < 3)
        return qQNaN();
    const double deg = parseRational(parts[0]);
    const double min = parseRational(parts[1]);
    const double sec = parseRational(parts[2]);
    return deg + min / 60.0 + sec / 3600.0;
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

std::optional<QPointF> Geo::parseGpsCoordinates(const QMap<QString, QString> &exifMap)
{
    const QString latStr = exifMap.value(QStringLiteral("GPSLatitude"));
    const QString latRef = exifMap.value(QStringLiteral("GPSLatitudeRef")).trimmed().toUpper();
    const QString lonStr = exifMap.value(QStringLiteral("GPSLongitude"));
    const QString lonRef = exifMap.value(QStringLiteral("GPSLongitudeRef")).trimmed().toUpper();

    if (latStr.isEmpty() || lonStr.isEmpty())
        return std::nullopt;

    double lat = dmsToDecimal(latStr);
    double lon = dmsToDecimal(lonStr);

    if (std::isnan(lat) || std::isnan(lon))
        return std::nullopt;

    if (latRef == QStringLiteral("S")) lat = -lat;
    if (lonRef == QStringLiteral("W")) lon = -lon;

    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        return std::nullopt;

    return QPointF(lat, lon);
}

QPoint Geo::latLonToTile(double lat, double lon, int zoom)
{
    const int    n      = 1 << zoom;
    const double latRad = lat * kPi / 180.0;
    const int    x      = static_cast<int>(std::floor((lon + 180.0) / 360.0 * n));
    const int    y      = static_cast<int>(std::floor(
        (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / kPi) / 2.0 * n));
    return {qBound(0, x, n - 1), qBound(0, y, n - 1)};
}

QPointF Geo::latLonToPixel(double lat, double lon, int zoom)
{
    const double n      = static_cast<double>(1 << zoom);
    const double latRad = lat * kPi / 180.0;
    const double px     = (lon + 180.0) / 360.0 * n * kTileSize;
    const double py     = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / kPi)
                          / 2.0 * n * kTileSize;
    return {px, py};
}

std::pair<double, double> Geo::pixelToLatLon(double px, double py, int zoom)
{
    const double n   = static_cast<double>(1 << zoom);
    const double lon = px / (n * kTileSize) * 360.0 - 180.0;
    const double lat = std::atan(std::sinh(kPi * (1.0 - 2.0 * py / (n * kTileSize))))
                       * 180.0 / kPi;
    return {lat, lon};
}

QString Geo::osmTileUrl(int x, int y, int zoom)
{
    return QString::fromLatin1(Compendia::OsmTileUrlTemplate)
        .replace(QStringLiteral("{z}"), QString::number(zoom))
        .replace(QStringLiteral("{x}"), QString::number(x))
        .replace(QStringLiteral("{y}"), QString::number(y));
}

void Geo::reverseGeocode(double lat, double lon,
                         QObject *context,
                         std::function<void(QString, QString, QString)> callback)
{
    MapTileCache::instance()->requestReverseGeocode(lat, lon, context, std::move(callback));
}
