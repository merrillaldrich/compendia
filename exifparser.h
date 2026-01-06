#ifndef EXIFPARSER_H
#define EXIFPARSER_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QByteArray>
#include <QMap>

#include <libheif/heif.h>

extern "C" {
#include "libexif/exif-data.h"
}

class ExifParser : public QObject
{
    Q_OBJECT

private:
    static ExifData* getExifStandard(QString filePath);
    static ExifData* getExifHeif(QString filePath);
    static QMap<QString, QString> exifTagsToMap(ExifData* ed);

public:
    explicit ExifParser(QObject *parent = nullptr);

    static QMap<QString, QString> getExifMap(QString filePath);

signals:
};

#endif // EXIFPARSER_H
