#include "maptilecache.h"
#include "geo.h"
#include "constants.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

MapTileCache *MapTileCache::instance()
{
    static MapTileCache inst;
    return &inst;
}

MapTileCache::MapTileCache(QObject *parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

QString MapTileCache::cacheKey(int x, int y, int zoom)
{
    return QString("%1/%2/%3").arg(zoom).arg(x).arg(y);
}

QPixmap MapTileCache::tilePixmap(int x, int y, int zoom)
{
    const QString key = cacheKey(x, y, zoom);

    if (tileCache_.contains(key))
        return *tileCache_.object(key);

    if (inFlight_.contains(key))
        return {};

    inFlight_.insert(key);

    QNetworkRequest req(QUrl(Geo::osmTileUrl(x, y, zoom)));
    req.setRawHeader("User-Agent", QByteArray(Compendia::MapNetworkUserAgent));

    QNetworkReply *reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, x, y, zoom, key]() {
        reply->deleteLater();
        inFlight_.remove(key);

        if (reply->error() != QNetworkReply::NoError)
            return;

        QPixmap pix;
        if (!pix.loadFromData(reply->readAll()))
            return;

        tileCache_.insert(key, new QPixmap(pix));
        emit tileReady(x, y, zoom, pix);
    });

    return {};
}

void MapTileCache::requestReverseGeocode(double lat, double lon,
                                          QObject *context,
                                          std::function<void(QString, QString, QString)> callback)
{
    QUrl url(QString::fromLatin1(Compendia::NominatimReverseUrl));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("lat"),    QString::number(lat, 'f', 7));
    q.addQueryItem(QStringLiteral("lon"),    QString::number(lon, 'f', 7));
    q.addQueryItem(QStringLiteral("zoom"),   QStringLiteral("10"));
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", QByteArray(Compendia::MapNetworkUserAgent));
    req.setRawHeader("Accept-Language", "en");

    QNetworkReply *reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [reply, context, cb = std::move(callback)]() mutable {
        reply->deleteLater();

        if (!context) {
            cb({}, {}, {});
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            cb({}, {}, {});
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonObject addr = root.value(QStringLiteral("address")).toObject();

        // Pick the most specific settlement name available
        QString city;
        for (const char *key : {"city", "town", "village", "hamlet", "suburb", "municipality"}) {
            const QString v = addr.value(QLatin1String(key)).toString();
            if (!v.isEmpty()) { city = v; break; }
        }

        const QString state   = addr.value(QStringLiteral("state")).toString();
        const QString country = addr.value(QStringLiteral("country")).toString();

        cb(city, state, country);
    });
}
