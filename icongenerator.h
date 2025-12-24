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

class IconGenerator : public QObject
{
    Q_OBJECT

public:
    IconGenerator(QObject *parent);
    static QImage generateIcon(const QString absoluteFileName);

private:
    static QImage makeSquareIcon(const QImage &source, int size);
    static bool saveIconToCache(const QString &absoluteFileName, const QImage &pict);
    static QImage loadIconFromCache(const QString &absoluteFileName);

};

#endif // ICONGENERATOR_H
