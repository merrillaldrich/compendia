#ifndef ICONGENERATOR_H
#define ICONGENERATOR_H

#include <QObject>
#include <QPixmap>
#include <QPainter>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QDebug>
#include <QVector>
#include <QImage>
#include <QAtomicInt>
#include <QMap>

#include "framegrabber.h"
#include "perceptualhasher.h"

/*! \brief Generates and caches scaled thumbnail images for media files (images and videos).
 *
 * Image files are processed in parallel via QtConcurrent. Video files are delegated to
 * FrameGrabber, which runs an async state machine on the main thread to avoid COM/WMF
 * apartment conflicts on Windows.
 *
 * Call processFiles() to begin a batch. Per-file results arrive via fileReady() on the main
 * thread. batchFinished() is emitted exactly once when all work is complete. Create a new
 * instance for each batch.
 */
class IconGenerator : public QObject
{
    Q_OBJECT

public:
    /*! \brief The discrete pixel sizes generated and cached for each media file. */
    static const QVector<int> kIconSizes;

    /*! \brief Constructs an IconGenerator.
     *
     * \param parent Qt parent object.
     */
    explicit IconGenerator(QObject *parent = nullptr);

    /*! \brief Begins thumbnail generation for all given file paths.
     *
     * Internally partitions paths into images and videos. Images are processed in parallel
     * via QtConcurrent; videos are handed to FrameGrabber on the main thread. Results arrive
     * via fileReady(); batchFinished() fires exactly once when everything is done.
     *
     * \param absolutePaths Absolute paths to image and/or video files to process.
     */
    void processFiles(const QStringList &absolutePaths);

    /*! \brief Returns true when all kIconSizes cache files exist and are newer than the source. */
    static bool             iconCacheValid(const QString &absolutePath);

    /*! \brief Loads all kIconSizes cached images for a file.
     *
     * \return Loaded images ordered by kIconSizes, or an empty vector on any failure.
     */
    static QVector<QImage>  loadIconsFromCache(const QString &absolutePath);

    /*! \brief Attempts to load a cached thumbnail for the given source file and size. */
    static QImage           loadIconFromCache(const QString &absoluteFileName, int size);

    /*! \brief Returns the absolute path to the .png cache file for a given source file and size. */
    static QString          cacheFilePath(const QString &absoluteFileName, int size);

signals:
    /*! \brief Emitted once per completed file (success). Always delivered on the main thread.
     *
     * \param absolutePath Absolute path to the source file.
     * \param exifMap      EXIF key-value data extracted from the file (empty for videos).
     * \param images       Scaled thumbnail images, one per kIconSizes entry.
     * \param pHash        Perceptual hash of the image (0 for videos).
     */
    void fileReady(const QString &absolutePath,
                   const QMap<QString, QString> &exifMap,
                   const QVector<QImage> &images,
                   quint64 pHash);

    /*! \brief Emitted exactly once when all image tasks and video grabs are complete. */
    void batchFinished();

private slots:
    /*! \brief Called (via QueuedConnection) when a background image task completes.
     *
     * \param path    Absolute path to the source image.
     * \param exifMap EXIF data read from the image.
     * \param images  Scaled thumbnail images.
     * \param pHash   Perceptual hash of the image.
     */
    void onImageTaskComplete(const QString &path,
                             const QMap<QString, QString> &exifMap,
                             const QVector<QImage> &images,
                             quint64 pHash);

    /*! \brief Called when FrameGrabber successfully captures a video frame.
     *
     * \param path     Absolute path to the video file.
     * \param rawFrame The captured frame (at most 400×400 px).
     * \param meta     Container metadata read from the video.
     */
    void onFrameGrabbed(const QString &path, const QImage &rawFrame,
                        const QMap<QString, QString> &meta);

    /*! \brief Called when FrameGrabber fails to capture a video frame.
     *
     * \param path   Absolute path to the video file.
     * \param reason Human-readable failure description.
     * \param meta   Container metadata (may be partially populated).
     */
    void onFrameFailed(const QString &path, const QString &reason,
                       const QMap<QString, QString> &meta);

    /*! \brief Called once FrameGrabber has finished all files in its batch.
     *
     * \param successCount Number of videos captured successfully.
     * \param failCount    Number of videos for which capture failed.
     */
    void onVideoGrabFinished(int successCount, int failCount);

private:
    /*! \brief Emits batchFinished() when both image tasks and video grabs are complete. */
    void checkBatchComplete();

    /*! \brief Processes an image file on a background thread (stateless).
     *
     * Checks cache first; on miss decodes via QImageReader, scales to all kIconSizes,
     * saves each to cache, and returns. Also extracts EXIF via ExifParser and
     * computes a perceptual hash from the largest available thumbnail.
     *
     * \param absolutePath Absolute path to the source image.
     * \return Tuple of (exifMap, images vector, pHash).
     */
    static std::tuple<QMap<QString, QString>, QVector<QImage>, quint64>
        processImageFile(const QString &absolutePath);

    /*! \brief Saves a thumbnail image to the per-folder cache directory.
     *
     * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
     * \param pict             The thumbnail image to save.
     * \param size             The pixel bound this thumbnail was scaled to.
     * \return True if the image was written successfully.
     */
    static bool saveIconToCache(const QString &absoluteFileName, const QImage &pict, int size);

    QAtomicInt    pendingImageCount_;        ///< Decremented as each image task completes.
    bool          videoGrabDone_ = true;     ///< True when no FrameGrabber work is pending.
    FrameGrabber *frameGrabber_  = nullptr;  ///< Active FrameGrabber, or nullptr.
};

#endif // ICONGENERATOR_H
