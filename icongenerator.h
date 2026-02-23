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

/*! \brief Utility class that generates and caches scaled thumbnail images for media files.
 *
 * Thumbnails are stored as serialised QImage files in a hidden
 * \c .luminism_cache subdirectory alongside each media file so that subsequent
 * loads skip the expensive image-decode step.
 */
class IconGenerator : public QObject
{
    Q_OBJECT

public:
    /*! \brief Constructs an IconGenerator.
     *
     * \param parent Qt parent object.
     */
    IconGenerator(QObject *parent);

    /*! \brief Returns a 100-pixel-bounded thumbnail for the given image file.
     *
     * Checks the on-disk cache first; on a miss the original image is decoded,
     * scaled, saved to the cache, and returned.
     *
     * \param absoluteFileName Absolute path to the source image file.
     * \return A QImage containing the scaled thumbnail, or a null QImage on error.
     */
    static QImage generateIcon(const QString absoluteFileName);

private:
    /*! \brief Saves a thumbnail image to the per-folder cache directory.
     *
     * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
     * \param pict             The thumbnail image to save.
     * \return True if the image was written successfully.
     */
    static bool saveIconToCache(const QString &absoluteFileName, const QImage &pict);

    /*! \brief Attempts to load a cached thumbnail for the given source file.
     *
     * \param absoluteFileName Absolute path to the original image (used to derive the cache path).
     * \return The cached QImage, or a null QImage if no valid cache entry exists.
     */
    static QImage loadIconFromCache(const QString &absoluteFileName);

};

#endif // ICONGENERATOR_H
