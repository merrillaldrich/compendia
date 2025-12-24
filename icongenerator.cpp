#include "icongenerator.h"

IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent){

}

QImage IconGenerator::generateIcon(const QString absoluteFileName)
{
    QImage iconPic;

    iconPic = loadIconFromCache(absoluteFileName);

    // For cache miss, process the original image, save the result to cache, and also return it
    if (iconPic.isNull()) {
        qDebug() << "Icon cache miss";
        QImage p = QImage(absoluteFileName);
        iconPic = makeSquareIcon(p, 100);

        bool cached = saveIconToCache(absoluteFileName, iconPic);

        if(cached)
            qDebug() << "Cached icon image";
    }
    return iconPic;
}

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

QImage IconGenerator::makeSquareIcon(const QImage &source, int size)
{
    // Scale the image to fit within a square, keeping aspect ratio
    QImage scaled = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    //// Create a transparent square pixmap
    //QImage square(size, size);
    //square.fill(Qt::transparent);

    //// Center the scaled image in the square
    //QPainter painter(&square);
    //int x = (size - scaled.width()) / 2;
    //int y = (size - scaled.height()) / 2;
    //painter.drawPixmap(x, y, scaled);
    //painter.end();

    QImage scaledImage = source.scaled(
        size, size,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    return scaledImage;
}
