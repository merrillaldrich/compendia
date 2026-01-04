#ifndef EXIFPARSER_H
#define EXIFPARSER_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QByteArray>

extern "C" {
#include "libexif/exif-data.h"
}

class ExifParser : public QObject
{
    Q_OBJECT

private:
    static void printExifContent(ExifContent *content, void *user_data);
    static void printExifEntry(ExifEntry *entry, void *user_data);

public:
    explicit ExifParser(QObject *parent = nullptr);

    static int readEXIF(QString filePath);

signals:
};

#endif // EXIFPARSER_H
