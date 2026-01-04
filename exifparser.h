#ifndef EXIFPARSER_H
#define EXIFPARSER_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QByteArray>

#include <libheif/heif.h>

extern "C" {
#include "libexif/exif-data.h"
}

class ExifParser : public QObject
{
    Q_OBJECT

private:
    static void printExifContent(ExifContent *content, void *user_data);
    static void printExifEntry(ExifEntry *entry, void *user_data);
    static void print_exif_tags(ExifData* ed);

public:
    explicit ExifParser(QObject *parent = nullptr);

    static int readEXIF(QString filePath);
    static int getExifHeif(QString absoluteFileName);

signals:
};

#endif // EXIFPARSER_H
