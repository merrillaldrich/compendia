#include "icongenerator.h"
#include <QImageReader>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>

/*! \brief The discrete pixel sizes generated and cached for each media file. */
const QVector<int> IconGenerator::kIconSizes = {50, 100, 200, 400};

/*! \brief Returns true if \a path has a recognised video file extension.
 *
 * \param path Absolute or relative file path to test.
 * \return True when the file extension matches a known video format.
 */
static bool isVideoFile(const QString &path)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    return videoExts.contains(QFileInfo(path).suffix().toLower());
}

/*! \brief Generates a fallback dark thumbnail with a white play triangle at the given size.
 *
 * Used when frame capture from a video file fails or times out.
 *
 * \param size Side length in pixels of the square placeholder image.
 * \return A QImage containing the placeholder icon.
 */
static QImage videoPlaceholderIcon(int size)
{
    QImage img(size, size, QImage::Format_RGB32);
    img.fill(QColor(30, 30, 30));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    // Scale triangle points proportionally: original was designed for 100px
    const double s = size / 100.0;
    QPointF pts[3] = { {35.0*s, 25.0*s}, {35.0*s, 75.0*s}, {75.0*s, 50.0*s} };
    p.drawPolygon(pts, 3);
    p.end();
    return img;
}

/*! \brief Seeks to ~2 seconds into a video file and returns the first decoded frame.
 *
 * Creates a QMediaPlayer and QVideoSink on the calling thread and runs a local
 * QEventLoop so that the asynchronous multimedia pipeline can deliver callbacks
 * without blocking the main event loop.  A 5-second timeout guards against
 * unresponsive or unsupported files.
 *
 * \param absoluteFileName Absolute path to the video file.
 * \return The captured frame scaled to at most 400×400 px, or a null QImage on failure.
 */
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
        result = result.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return result;
}

/*! \brief Composites filmstrip and play-button decorations onto a video thumbnail.
 *
 * Paints dark vertical bands along the left and right edges with evenly-spaced
 * near-white sprocket holes centred vertically, then draws a semi-transparent
 * play-button disc and triangle in the centre of the image.  All measurements
 * are proportional to the image width so the decoration looks correct at any
 * of the four cached sizes.
 *
 * \param source The thumbnail frame to decorate (any square size).
 * \return A new QImage with the decorations composited over \a source.
 */
static QImage overlayVideoDecorations(const QImage &source)
{
    QImage img = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    const int w     = img.width();
    const int h     = img.height();
    const int bandW = qMax(3, w / 10);  // at 100px → 10, at 400px → 40

    // --- Filmstrip bands (left and right) ---
    p.fillRect(0,         0, bandW, h, QColor(15, 15, 15, 225));
    p.fillRect(w - bandW, 0, bandW, h, QColor(15, 15, 15, 225));

    // Sprocket holes: proportional size, centred vertically inside each side band
    const int    holeW      = qMax(2, bandW * 3 / 5);   // at 100px → 6
    const int    holeH      = holeW;                     // square holes
    const int    holeGap    = qMax(1, holeH * 2 / 3);   // at 100px → 4
    const double holeRadius = holeW * 0.25;              // at 100px → 1.5
    const int    numHoles   = qMin(6, h / qMax(1, holeH + holeGap));
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

    // --- Play button (proportional to image width) ---
    const double cx = w * 0.5;
    const double cy = h * 0.5;

    // Semi-transparent dark disc — keeps the triangle readable over bright frames
    const double discR = w * 0.085;  // at 100px → 8.5
    p.setBrush(QColor(0, 0, 0, 110));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, cy), discR, discR);

    // White triangle: nudge right so its visual centroid sits at (cx, cy)
    const double th = w * 0.07;           // half-height; at 100px → 7.0
    const double tl = cx - th * 0.50;    // left edge x
    const double tr = cx + th * 0.90;    // tip x
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

/*! \brief Returns the absolute path to the .qimg cache file for a given source image and size.
 *
 * \param absoluteFileName Absolute path to the source image file.
 * \param size             The pixel bound embedded in the cache file name.
 * \return Absolute path to the corresponding cache file.
 */
QString IconGenerator::cacheFilePath(const QString &absoluteFileName, int size)
{
    QFileInfo fi(absoluteFileName);
    return fi.absolutePath() + "/.luminism_cache/" + fi.baseName()
           + "_" + QString::number(size) + ".qimg";
}

/*! \brief Returns thumbnails at all cached sizes for the given media file.
 *
 * Checks the on-disk cache for every size first.  If all four caches are
 * current (newer than the source file) they are loaded and returned without
 * re-decoding the source.  On any miss the source is decoded once (or a video
 * frame captured at 400 px), scaled to each of the four sizes, cached, and
 * returned as a QVector ordered smallest-to-largest.
 *
 * \param absoluteFileName Absolute path to the source media file.
 * \return A QVector of QImages (one per kIconSizes entry), or an empty vector
 *         on error.
 */
QVector<QImage> IconGenerator::generateIcon(const QString absoluteFileName)
{
    QFileInfo sourceInfo(absoluteFileName);

    // Check whether all four size caches are current
    bool allCached = true;
    for (int size : kIconSizes) {
        QFileInfo cacheInfo(cacheFilePath(absoluteFileName, size));
        if (!cacheInfo.exists() || cacheInfo.lastModified() < sourceInfo.lastModified()) {
            allCached = false;
            break;
        }
    }

    if (allCached) {
        QVector<QImage> images;
        bool allLoaded = true;
        for (int size : kIconSizes) {
            QImage img = loadIconFromCache(absoluteFileName, size);
            if (img.isNull()) { allLoaded = false; break; }
            images.append(img);
        }
        if (allLoaded) return images;
    }

    qDebug() << "Icon cache miss";

    // Decode/capture once at the largest size, then scale down
    QImage base;
    if (isVideoFile(absoluteFileName)) {
        base = captureVideoFrame(absoluteFileName);  // already scaled to ≤400px
        if (!base.isNull())
            base = overlayVideoDecorations(base);
        else
            base = videoPlaceholderIcon(400);
    } else {
        QImageReader ir(absoluteFileName);
        ir.setAutoTransform(true);
        base = ir.read();
        base = base.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (base.isNull())
        return {};

    QVector<QImage> images;
    for (int size : kIconSizes) {
        QImage scaled = base.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        saveIconToCache(absoluteFileName, scaled, size);
        images.append(scaled);
    }

    return images;
}

/*! \brief Saves a thumbnail image to the per-folder cache directory.
 *
 * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
 * \param pict             The thumbnail image to save.
 * \param size             The pixel bound this thumbnail was scaled to.
 * \return True if the image was written successfully.
 */
bool IconGenerator::saveIconToCache(const QString &absoluteFileName, const QImage &pict, int size) {

    QFileInfo fi(absoluteFileName);
    QString cachePath = fi.absolutePath() + "/.luminism_cache";
    QDir dir(cachePath);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create directory:" << cachePath;
            return false;
        }
    }

    QString filePath = cacheFilePath(absoluteFileName, size);

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
 * \param size             The pixel bound of the cached thumbnail to load.
 * \return The cached QImage, or a null QImage if no valid cache entry exists.
 */
QImage IconGenerator::loadIconFromCache(const QString &absoluteFileName, int size){

    QString filePath = cacheFilePath(absoluteFileName, size);

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
