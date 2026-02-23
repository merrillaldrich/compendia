#include "icongenerator.h"
#include <qimagereader.h>

/*! \brief Constructs an IconGenerator.
 *
 * \param parent Qt parent object.
 */
IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent){

}

/*! \brief Returns a 100-pixel-bounded thumbnail for the given image file.
 *
 * Checks the on-disk cache first; on a miss the original image is decoded,
 * scaled, saved to the cache, and returned.
 *
 * \param absoluteFileName Absolute path to the source image file.
 * \return A QImage containing the scaled thumbnail, or a null QImage on error.
 */
QImage IconGenerator::generateIcon(const QString absoluteFileName)
{
    QImage iconPic;

    iconPic = loadIconFromCache(absoluteFileName);

    // For cache miss, process the original image, save the result to cache, and also return it
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
    QString cachePath = fi.absolutePath() + "/" + ".luminism_cache";
    QDir dir(cachePath);

    if (!dir.exists()) {
        if (dir.mkpath(".")) {
        } else {
            qWarning() << "Failed to create directory:" << cachePath;
            return false;
        }
    }

    QString filePath = cachePath + "/" + fi.baseName() + "_100" + ".qimg";

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

    QFileInfo fi(absoluteFileName);
    QString cachePath = fi.absolutePath() + "/" + ".luminism_cache";
    QDir dir(cachePath);

    if (!dir.exists()) {
        qWarning() << "There is no cache folder for absoluteFileName";
        return QImage();
    }

    QString filePath = cachePath + "/" + fi.baseName() + "_100" + ".qimg";

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
