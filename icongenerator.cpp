#include "icongenerator.h"

IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent){

}

QPixmap IconGenerator::generateIcon(const QString absoluteFileName)
{
    QPixmap iconPic;

    iconPic = loadIconFromCache(absoluteFileName);

    // For cache miss, process the original image, save the result to cache, and also return it
    if (iconPic.isNull()) {
        qDebug() << "Icon cache miss";
        QPixmap p = QPixmap(absoluteFileName);
        iconPic = makeSquareIcon(p, 100);

        bool cached = saveIconToCache(absoluteFileName, iconPic);

        if(cached)
            qDebug() << "Cached icon image";
    }
    return iconPic;
}

bool IconGenerator::saveIconToCache(const QString &absoluteFileName, const QPixmap &pict) {

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

    QString filePath = cachePath + "/" + fi.baseName() + "_100" + ".pixmap";

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

QPixmap IconGenerator::loadIconFromCache(const QString &absoluteFileName){

    QFileInfo fi(absoluteFileName);
    QString cachePath = fi.absolutePath() + "/" + ".luminism_cache";
    QDir dir(cachePath);

    if (!dir.exists()) {
        qWarning() << "There is no cache folder for absoluteFileName";
        return QPixmap();
    }

    QString filePath = cachePath + "/" + fi.baseName() + "_100" + ".pixmap";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open cache file for reading:" << file.errorString();
        return QPixmap();
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    QPixmap iconPic;
    in >> iconPic;

    return iconPic;
}

QPixmap IconGenerator::makeSquareIcon(const QPixmap &source, int size)
{
    // Scale the image to fit within a square, keeping aspect ratio
    QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Create a transparent square pixmap
    QPixmap square(size, size);
    square.fill(Qt::transparent);

    // Center the scaled image in the square
    QPainter painter(&square);
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.end();

    return square;
}
