#include "icongenerator.h"
#include <qimagereader.h>

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

        int size = 100;
        QImageReader ir(absoluteFileName);
        ir.setAutoTransform(true);
        iconPic = ir.read();
        iconPic = iconPic.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
