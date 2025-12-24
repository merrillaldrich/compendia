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
    static QPixmap generateIcon(const QString absoluteFileName);

private:
    static QPixmap makeSquareIcon(const QPixmap &source, int size);
    static bool saveIconToCache(const QString &absoluteFileName, const QPixmap &pict);
    static QPixmap loadIconFromCache(const QString &absoluteFileName);

};

#endif // ICONGENERATOR_H
