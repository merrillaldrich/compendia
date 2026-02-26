#include "icongenerator.h"
#include <QImageReader>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>

static bool isVideoFile(const QString &path)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    return videoExts.contains(QFileInfo(path).suffix().toLower());
}

static QImage videoPlaceholderIcon()
{
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(QColor(30, 30, 30));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    QPointF pts[3] = { {35.0, 25.0}, {35.0, 75.0}, {75.0, 50.0} };
    p.drawPolygon(pts, 3);
    p.end();
    return img;
}

// Seek to a short time into the video and capture the first frame that arrives.
// Returns a null QImage if the video cannot be decoded or times out.
static QImage captureVideoFrame(const QString &absoluteFileName)
{
    QImage result;
    QEventLoop loop;

    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);

    bool frameReceived = false;

    // Capture the first valid frame from the sink.
    // Qt::AutoConnection: queued when emitted from an internal multimedia thread,
    // so the lambda runs safely on this thread inside loop.exec().
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &sink,
                     [&](const QVideoFrame &frame) {
        if (!frameReceived && frame.isValid()) {
            frameReceived = true;
            QVideoFrame f = frame;
            result = f.toImage();
            player.stop();
            loop.quit();
        }
    });

    // Once the media is loaded, seek to ~2 s and start playing so frames arrive.
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &player,
                     [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            qint64 dur = player.duration();
            qint64 seekPos = (dur <= 0 || dur > 2000) ? 2000 : dur / 4;
            player.setPosition(seekPos);
            player.play();
        } else if (status == QMediaPlayer::EndOfMedia
                   || status == QMediaPlayer::InvalidMedia) {
            loop.quit();
        }
    });

    QObject::connect(&player, &QMediaPlayer::errorOccurred, &player,
                     [&](QMediaPlayer::Error, const QString &) {
        loop.quit();
    });

    // Safety net: give up after 5 s regardless.
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(5000);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    player.setSource(QUrl::fromLocalFile(absoluteFileName));
    timeout.start();
    loop.exec();  // process multimedia events on this thread until a frame arrives

    if (!result.isNull())
        result = result.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return result;
}

// Paints filmstrip bands (left + right) and a centred play-button triangle
// over a 100×100 video thumbnail.
static QImage overlayVideoDecorations(const QImage &source)
{
    QImage img = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    const int w     = img.width();
    const int h     = img.height();
    const int bandW = 10;  // 13 * 0.75 ≈ 10 px — side bands

    // --- Filmstrip bands (left and right) ---
    p.fillRect(0,         0, bandW, h, QColor(15, 15, 15, 225));
    p.fillRect(w - bandW, 0, bandW, h, QColor(15, 15, 15, 225));

    // Sprocket holes: fixed-gap group centred vertically inside each side band
    const int    holeW      = 6;
    const int    holeH      = 6;
    const int    numHoles   = 6;
    const int    holeGap    = 4;
    const double holeRadius = 1.5;
    const int    blockH     = numHoles * holeH + (numHoles - 1) * holeGap;
    const int    startY     = (h - blockH) / 2;
    const int    xLeft      = (bandW - holeW) / 2;
    const int    xRight     = w - bandW + (bandW - holeW) / 2;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(230, 230, 230, 210));
    for (int i = 0; i < numHoles; ++i) {
        int y = startY + i * (holeH + holeGap);
        p.drawRoundedRect(QRectF(xLeft,  y, holeW, holeH), holeRadius, holeRadius);
        p.drawRoundedRect(QRectF(xRight, y, holeW, holeH), holeRadius, holeRadius);
    }

    // --- Play button (50 % smaller than original) ---
    const double cx = w * 0.5;
    const double cy = h * 0.5;

    // Semi-transparent dark disc — keeps the triangle readable over bright frames
    p.setBrush(QColor(0, 0, 0, 110));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, cy), 8.5, 8.5);

    // White triangle: nudge right so its visual centroid sits at (cx, cy)
    const double th = 7.0;             // half-height of triangle
    const double tl = cx - th * 0.50; // left edge x
    const double tr = cx + th * 0.90; // tip x
    QPointF pts[3] = {
        { tl, cy - th },
        { tl, cy + th },
        { tr, cy      },
    };
    p.setBrush(QColor(255, 255, 255, 215));
    p.drawPolygon(pts, 3);

    p.end();
    return img;
}

/*! \brief Constructs an IconGenerator.
 *
 * \param parent Qt parent object.
 */
IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent){

}

/*! \brief Returns the absolute path to the .qimg cache file for a given source image.
 *
 * \param absoluteFileName Absolute path to the source image file.
 * \return Absolute path to the corresponding cache file.
 */
QString IconGenerator::cacheFilePath(const QString &absoluteFileName)
{
    QFileInfo fi(absoluteFileName);
    return fi.absolutePath() + "/.luminism_cache/" + fi.baseName() + "_100.qimg";
}

/*! \brief Returns a 100-pixel-bounded thumbnail for the given image file.
 *
 * Checks the on-disk cache first. The cache is bypassed when the source image
 * has been modified more recently than the cached file. On a miss or stale
 * cache the original image is decoded, scaled, saved to cache, and returned.
 *
 * \param absoluteFileName Absolute path to the source image file.
 * \return A QImage containing the scaled thumbnail, or a null QImage on error.
 */
QImage IconGenerator::generateIcon(const QString absoluteFileName)
{
    QImage iconPic;

    QFileInfo sourceInfo(absoluteFileName);
    QFileInfo cacheInfo(cacheFilePath(absoluteFileName));

    bool cacheCurrent = cacheInfo.exists() &&
                        cacheInfo.lastModified() >= sourceInfo.lastModified();

    if (cacheCurrent)
        iconPic = loadIconFromCache(absoluteFileName);

    if (iconPic.isNull()) {
        qDebug() << "Icon cache miss";

        if (isVideoFile(absoluteFileName)) {
            iconPic = captureVideoFrame(absoluteFileName);
            if (!iconPic.isNull())
                iconPic = overlayVideoDecorations(iconPic);
            else
                iconPic = videoPlaceholderIcon();
        } else {
            int size = 100;
            QImageReader ir(absoluteFileName);
            ir.setAutoTransform(true);
            iconPic = ir.read();
            iconPic = iconPic.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        bool cached = saveIconToCache(absoluteFileName, iconPic);

        if(cached)
            qDebug() << "Cached icon image";
    }

    return iconPic;
}

/*! \brief Saves a thumbnail image to the per-folder cache directory.
 *
 * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
 * \param pict             The thumbnail image to save.
 * \return True if the image was written successfully.
 */
bool IconGenerator::saveIconToCache(const QString &absoluteFileName, const QImage &pict) {

    QFileInfo fi(absoluteFileName);
    QString cachePath = fi.absolutePath() + "/.luminism_cache";
    QDir dir(cachePath);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create directory:" << cachePath;
            return false;
        }
    }

    QString filePath = cacheFilePath(absoluteFileName);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file for writing:" << file.errorString();
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0); // Ensure compatibility
    out << pict;

    return true;
}

/*! \brief Attempts to load a cached thumbnail for the given source file.
 *
 * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
 * \return The cached QImage, or a null QImage if no valid cache entry exists.
 */
QImage IconGenerator::loadIconFromCache(const QString &absoluteFileName){

    QString filePath = cacheFilePath(absoluteFileName);

    if (!QFile::exists(filePath)) {
        qWarning() << "There is no cache file for" << absoluteFileName;
        return QImage();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open cache file for reading:" << file.errorString();
        return QImage();
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    QImage iconPic;
    in >> iconPic;

    return iconPic;
}
