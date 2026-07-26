#include "maptilecache.h"
#include "geo.h"
#include "constants.h"
#include "secrets.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

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

    QNetworkRequest req(QUrl(Geo::tileUrl(x, y, zoom)));
    req.setRawHeader("User-Agent", QByteArray(Compendia::MapNetworkUserAgent));
    {
        QSettings qs(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
        if (qs.value(Compendia::MapUseFreeTierSettingsKey, false).toBool())
            req.setRawHeader("X-Compendia-Key", QByteArray(COMPENDIA_PROXY_APP_KEY));
    }

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
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
    const bool freeTier = s.value(Compendia::MapUseFreeTierSettingsKey, false).toBool();

    QNetworkRequest req;
    if (freeTier) {
        const QString urlStr = QString::fromLatin1(Compendia::CompendiaGeocodeProxyUrl)
            + QString("?lat=%1&lon=%2")
                .arg(QString::number(lat, 'f', 7))
                .arg(QString::number(lon, 'f', 7));
        req = QNetworkRequest{QUrl(urlStr)};
        req.setRawHeader("X-Compendia-Key", QByteArray(COMPENDIA_PROXY_APP_KEY));
    } else {
        const QString token = s.value(Compendia::MapApiTokenSettingsKey).toString();
        const QString urlStr = QString::fromLatin1(Compendia::MapboxReverseGeoUrl)
            .replace(QStringLiteral("{lon}"),   QString::number(lon, 'f', 7))
            .replace(QStringLiteral("{lat}"),   QString::number(lat, 'f', 7))
            .replace(QStringLiteral("{token}"), token);
        req = QNetworkRequest{QUrl(urlStr)};
    }
    req.setRawHeader("User-Agent", QByteArray(Compendia::MapNetworkUserAgent));

    QNetworkReply *reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this,
            [reply, context, cb = std::move(callback)]() mutable {
        reply->deleteLater();

        if (!context || reply->error() != QNetworkReply::NoError) {
            cb({}, {}, {});
            return;
        }

        // Parse Mapbox GeoJSON FeatureCollection
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray features = root.value(QStringLiteral("features")).toArray();
        if (features.isEmpty()) { cb({}, {}, {}); return; }

        const QJsonArray contextArr =
            features[0].toObject().value(QStringLiteral("context")).toArray();

        QString city, state, country;
        for (const QJsonValue &v : contextArr) {
            const QJsonObject obj  = v.toObject();
            const QString     id   = obj.value(QStringLiteral("id")).toString();
            const QString     text = obj.value(QStringLiteral("text")).toString();
            if      (id.startsWith(QLatin1String("place.")))    city    = text;
            else if (id.startsWith(QLatin1String("locality."))) { if (city.isEmpty()) city = text; }
            else if (id.startsWith(QLatin1String("region.")))   state   = text;
            else if (id.startsWith(QLatin1String("country.")))  country = text;
        }

        cb(city, state, country);
    });
}
