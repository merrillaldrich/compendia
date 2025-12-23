#include "icongenerator.h"

IconGenerator::IconGenerator(QObject *parent)
    : QObject(parent){

}

QPixmap IconGenerator::generateIcon(const QString absoluteFileName)
{
    QPixmap p = QPixmap(absoluteFileName);
    QPixmap squarePixmap = makeSquareIcon(p, 100);
    return squarePixmap;
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
