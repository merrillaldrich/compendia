#include "icongenerator.h"
#include "constants.h"
#include "exifparser.h"
#include <QImageReader>
#include <QFileInfo>
#include <QTimer>
#include <QtConcurrent>
#include <utility>

/*! \brief The discrete pixel sizes generated and cached for each media file. */
const QVector<int> IconGenerator::kIconSizes = {100, 200};

/*! \brief Composites filmstrip and play-button decorations onto a video thumbnail.
 *
 * Paints dark vertical bands along the left and right edges with evenly-spaced
 * near-white sprocket holes centred vertically, then draws a semi-transparent
 * play-button disc and triangle in the centre of the image.  All measurements
 * are proportional to the image width so the decoration looks correct at any
 * of the four cached sizes.
 *
 * \param source The thumbnail frame to decorate.
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

/*! \brief Generates a placeholder thumbnail for a video whose frame capture failed.
 *
 * Creates a dark 16:9 base image at the given width and applies the same filmstrip
 * and play-button decorations as a successful video thumbnail, so the placeholder
 * is visually consistent with real video icons.
 *
 * \param size Width in pixels of the placeholder (height is derived as 9/16 of width).
 * \return A decorated QImage in landscape proportions.
 */
static QImage videoPlaceholderIcon(int size)
{
    const int w = size;
    const int h = qMax(1, size * 9 / 16);
    QImage base(w, h, QImage::Format_RGB32);
    base.fill(QColor(30, 30, 30));
    return overlayVideoDecorations(base);
}

/*! \brief Constructs an IconGenerator.
 *
 * \param parent Qt parent object.
 */
IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent)
{
}

/*! \brief Begins thumbnail generation for all given file paths. */
void IconGenerator::processFiles(const QStringList &absolutePaths)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};

    if (absolutePaths.isEmpty()) {
        pendingImageCount_.storeRelaxed(0);
        videoGrabDone_ = true;
        QTimer::singleShot(0, this, &IconGenerator::checkBatchComplete);
        return;
    }

    QStringList imagePaths, videoPaths;
    for (const QString &path : absolutePaths) {
        if (videoExts.contains(QFileInfo(path).suffix().toLower()))
            videoPaths << path;
        else
            imagePaths << path;
    }

    pendingImageCount_.storeRelaxed(imagePaths.size());
    videoGrabDone_ = videoPaths.isEmpty();

    // Force Qt to load and register all image format plugins NOW, on the main
    // thread, before any background tasks start.  On Linux, plugin discovery
    // touches global plugin-loader state that is not re-entrant, so if multiple
    // QtConcurrent threads hit QImageReader::read() simultaneously before the
    // registry is populated the first call can race and corrupt it, leading to
    // a crash inside QImage.  Calling supportedImageFormats() here is a no-op
    // on subsequent calls once the registry is warm.
    QImageReader::supportedImageFormats();

    // --- Video handling ---
    QStringList videoGrabNeeded;
    for (const QString &path : videoPaths) {
        if (iconCacheValid(path)) {
            QVector<QImage> cached = loadIconsFromCache(path);
            if (!cached.isEmpty())
                emit fileReady(path, {}, cached, 0);
            else
                videoGrabNeeded << path;
        } else {
            videoGrabNeeded << path;
        }
    }

    if (!videoPaths.isEmpty() && videoGrabNeeded.isEmpty())
        videoGrabDone_ = true;

    if (!videoGrabNeeded.isEmpty()) {
        frameGrabber_ = new FrameGrabber(this);
        connect(frameGrabber_, &FrameGrabber::frameGrabbed, this, &IconGenerator::onFrameGrabbed);
        connect(frameGrabber_, &FrameGrabber::frameFailed,  this, &IconGenerator::onFrameFailed);
        connect(frameGrabber_, &FrameGrabber::finished,     this, &IconGenerator::onVideoGrabFinished);
        frameGrabber_->grab(videoGrabNeeded);
    }

    // --- Image handling ---
    for (const QString &path : imagePaths) {
        QtConcurrent::run([this, path]() {
            auto [exifMap, images, pHash] = processImageFile(path);
            QMetaObject::invokeMethod(this, [this, path, exifMap, images, pHash]() {
                onImageTaskComplete(path, exifMap, images, pHash);
            }, Qt::QueuedConnection);
        });
    }

    checkBatchComplete();
}

/*! \brief Called when a background image task completes.
 *
 * \param path    Absolute path to the source image.
 * \param exifMap EXIF data read from the image.
 * \param images  Scaled thumbnail images.
 */
void IconGenerator::onImageTaskComplete(const QString &path,
                                        const QMap<QString, QString> &exifMap,
                                        const QVector<QImage> &images,
                                        quint64 pHash)
{
    emit fileReady(path, exifMap, images, pHash);
    if (pendingImageCount_.fetchAndAddOrdered(-1) - 1 == 0)
        checkBatchComplete();
}

/*! \brief Called when FrameGrabber successfully captures a video frame.
 *
 * \param path     Absolute path to the video file.
 * \param rawFrame The captured frame (at most 400×400 px).
 * \param meta     Container metadata read from the video.
 */
void IconGenerator::onFrameGrabbed(const QString &path, const QImage &rawFrame,
                                   const QMap<QString, QString> &meta)
{
    QImage decorated = overlayVideoDecorations(rawFrame);
    QVector<QImage> images;
    for (int size : kIconSizes) {
        QImage scaled = decorated.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        saveIconToCache(path, scaled, size);
        images << scaled;
    }
    emit fileReady(path, meta, images, 0);
}

/*! \brief Called when FrameGrabber fails to capture a video frame.
 *
 * \param path   Absolute path to the video file.
 * \param reason Human-readable failure description.
 * \param meta   Container metadata (may be partially populated).
 */
void IconGenerator::onFrameFailed(const QString &path, const QString &reason,
                                  const QMap<QString, QString> &meta)
{
    qWarning() << "IconGenerator: frame capture failed for" << path << ":" << reason;
    QVector<QImage> placeholders;
    for (int size : kIconSizes)
        placeholders << videoPlaceholderIcon(size);
    emit fileReady(path, meta, placeholders, 0);
}

/*! \brief Called when FrameGrabber finishes all files in its batch.
 *
 * \param successCount Number of videos captured successfully.
 * \param failCount    Number of videos for which capture failed.
 */
void IconGenerator::onVideoGrabFinished(int successCount, int failCount)
{
    Q_UNUSED(successCount)
    Q_UNUSED(failCount)
    videoGrabDone_ = true;
    frameGrabber_->deleteLater();
    frameGrabber_ = nullptr;
    checkBatchComplete();
}

/*! \brief Emits batchFinished() when both image tasks and video grabs are complete. */
void IconGenerator::checkBatchComplete()
{
    if (pendingImageCount_.loadAcquire() != 0) return;
    if (!videoGrabDone_) return;
    emit batchFinished();
}

/*! \brief Processes an image file on a background thread (stateless).
 *
 * \param absolutePath Absolute path to the source image.
 * \return Tuple of (exifMap, images vector, pHash).
 */
std::tuple<QMap<QString, QString>, QVector<QImage>, quint64>
IconGenerator::processImageFile(const QString &absolutePath)
{
    QFileInfo sourceInfo(absolutePath);

    // Check whether all four size caches are current
    bool allCached = true;
    for (int size : kIconSizes) {
        QFileInfo cacheInfo(cacheFilePath(absolutePath, size));
        if (!cacheInfo.exists() || cacheInfo.lastModified() < sourceInfo.lastModified()) {
            allCached = false;
            break;
        }
    }

    if (allCached) {
        QVector<QImage> images;
        bool allLoaded = true;
        for (int size : kIconSizes) {
            QImage img = loadIconFromCache(absolutePath, size);
            if (img.isNull()) { allLoaded = false; break; }
            images.append(img);
        }
        if (allLoaded) {
            quint64 pHash = PerceptualHasher::computeHash(images.last());
            return {ExifParser::getExifMap(absolutePath), images, pHash};
        }
    }

    qDebug() << "Icon cache miss:" << absolutePath;

    QImage base;
    if (QFileInfo(absolutePath).suffix().toLower() == "heic") {
        base = ExifParser::loadHeifImage(absolutePath);
    } else {
        QImageReader ir(absolutePath);
        ir.setAutoTransform(true);
        base = ir.read();
    }
    if (base.isNull())
        return {{}, {}, 0};

    base = base.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QVector<QImage> images;
    for (int size : kIconSizes) {
        QImage scaled = base.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        saveIconToCache(absolutePath, scaled, size);
        images.append(scaled);
    }

    quint64 pHash = PerceptualHasher::computeHash(images.last());
    return {ExifParser::getExifMap(absolutePath), images, pHash};
}

/*! \brief Returns true when all kIconSizes cache files exist and are newer than the source.
 *
 * \param absolutePath Absolute path to the source file.
 * \return True if the full cache is valid.
 */
bool IconGenerator::iconCacheValid(const QString &absolutePath)
{
    QFileInfo sourceInfo(absolutePath);
    for (int size : kIconSizes) {
        QFileInfo cacheInfo(cacheFilePath(absolutePath, size));
        if (!cacheInfo.exists() || cacheInfo.lastModified() < sourceInfo.lastModified())
            return false;
    }
    return true;
}

/*! \brief Loads all kIconSizes cached images for a file.
 *
 * \param absolutePath Absolute path to the source file.
 * \return Loaded images ordered by kIconSizes, or an empty vector on any failure.
 */
QVector<QImage> IconGenerator::loadIconsFromCache(const QString &absolutePath)
{
    QVector<QImage> images;
    for (int size : kIconSizes) {
        QImage img = loadIconFromCache(absolutePath, size);
        if (img.isNull())
            return {};
        images.append(img);
    }
    return images;
}

/*! \brief Returns the absolute path to the PNG cache file for a given source image and size.
 *
 * The full file name (including extension) is used in the cache key so that
 * files with the same stem but different extensions (e.g. photo.jpg and
 * photo.heic) do not collide.
 *
 * \param absoluteFileName Absolute path to the source image file.
 * \param size             The pixel bound embedded in the cache file name.
 * \return Absolute path to the corresponding cache file.
 */
QString IconGenerator::cacheFilePath(const QString &absoluteFileName, int size)
{
    QFileInfo fi(absoluteFileName);
    return fi.absolutePath() + "/" + Compendia::CacheFolderName + "/" + fi.fileName()
           + "_" + QString::number(size) + ".png";
}

/*! \brief Saves a thumbnail image to the per-folder cache directory.
 *
 * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
 * \param pict             The thumbnail image to save.
 * \param size             The pixel bound this thumbnail was scaled to.
 * \return True if the image was written successfully.
 */
bool IconGenerator::saveIconToCache(const QString &absoluteFileName, const QImage &pict, int size)
{
    QFileInfo fi(absoluteFileName);
    QString cachePath = fi.absolutePath() + "/" + Compendia::CacheFolderName;
    QDir dir(cachePath);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create directory:" << cachePath;
            return false;
        }
    }

    QString filePath = cacheFilePath(absoluteFileName, size);

    if (!pict.save(filePath, "PNG")) {
        qWarning() << "Cannot save icon cache file:" << filePath;
        return false;
    }

    return true;
}

/*! \brief Attempts to load a cached thumbnail for the given source file.
 *
 * \param absoluteFileName Absolute path to the original image.
 * \param size             The pixel bound of the cached thumbnail to load.
 * \return The cached QImage, or a null QImage if no valid cache entry exists.
 */
QImage IconGenerator::loadIconFromCache(const QString &absoluteFileName, int size)
{
    QString filePath = cacheFilePath(absoluteFileName, size);

    if (!QFile::exists(filePath)) {
        qWarning() << "There is no cache file for" << absoluteFileName;
        return QImage();
    }

    QImage iconPic;
    if (!iconPic.load(filePath)) {
        qWarning() << "Cannot load icon cache file:" << filePath;
        return QImage();
    }

    return iconPic;
}
