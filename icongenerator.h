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

/*! \brief Utility class that generates and caches scaled thumbnail images for media files.
 *
 * Thumbnails are stored as serialised QImage files in a hidden
 * \c .luminism_cache subdirectory alongside each media file so that subsequent
 * loads skip the expensive image-decode step.  Four discrete sizes are generated
 * and cached (50, 100, 200, 400 px) so that Qt can always downscale to any zoom
 * level without upscaling artefacts.
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
    IconGenerator(QObject *parent);

    /*! \brief Returns thumbnails at all cached sizes for the given media file.
     *
     * Checks the on-disk cache for every size first; if all caches are current
     * they are loaded and returned immediately.  On any miss the source is
     * decoded once, scaled to each size, saved, and returned.
     *
     * \param absoluteFileName Absolute path to the source media file.
     * \return A QVector of QImages (one per entry in kIconSizes), or an empty
     *         vector on error.
     */
    static QVector<QImage> generateIcon(const QString absoluteFileName);

private:
    /*! \brief Saves a thumbnail image to the per-folder cache directory.
     *
     * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
     * \param pict             The thumbnail image to save.
     * \param size             The pixel bound this thumbnail was scaled to.
     * \return True if the image was written successfully.
     */
    static bool saveIconToCache(const QString &absoluteFileName, const QImage &pict, int size);

    /*! \brief Attempts to load a cached thumbnail for the given source file.
     *
     * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
     * \param size             The pixel bound of the cached thumbnail to load.
     * \return The cached QImage, or a null QImage if no valid cache entry exists.
     */
    static QImage loadIconFromCache(const QString &absoluteFileName, int size);

    /*! \brief Returns the absolute path to the .qimg cache file for a given source image and size.
     *
     * \param absoluteFileName Absolute path to the source image file.
     * \param size             The pixel bound embedded in the cache file name.
     * \return Absolute path to the corresponding cache file.
     */
    static QString cacheFilePath(const QString &absoluteFileName, int size);

};

#endif // ICONGENERATOR_H
