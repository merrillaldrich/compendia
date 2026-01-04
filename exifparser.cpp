#include "exifparser.h"

ExifParser::ExifParser(QObject *parent)
    : QObject{parent}
{}

int ExifParser::readEXIF(QString filePath){

    // Read file into memory
    QFile file(filePath);
    if (!file.exists()) {
        qCritical() << "File does not exist:" << filePath;
        return 1;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << file.errorString();
        return 1;
    }

    QByteArray imageData = file.readAll();
    file.close();

    if (imageData.isEmpty()) {
        qCritical() << "File is empty or could not be read.";
        return 1;
    }

    // Load EXIF data from memory
    ExifData *exifData = exif_data_new_from_data(
        reinterpret_cast<const unsigned char*>(imageData.constData()),
        static_cast<unsigned int>(imageData.size())
        );

    if (!exifData) {
        qCritical() << "No EXIF data found in file.";
        return 1;
    }

    // Iterate and print EXIF tags
    exif_data_foreach_content(exifData, printExifContent, nullptr);

    // Free EXIF data
    exif_data_unref(exifData);

    return 0;
}

// Iterate over all EXIF content
void ExifParser::printExifContent(ExifContent *content, void *user_data) {
    exif_content_foreach_entry(content, printExifEntry, user_data);
}

// Callback function for iterating over EXIF entries
void ExifParser::printExifEntry(ExifEntry *entry, void *user_data) {
    char value[1024];
    exif_entry_get_value(entry, value, sizeof(value));

    if (*value) { // Only print if value is not empty
        QString tagName = QString::fromUtf8(exif_tag_get_name(entry->tag));
        QString tagValue = QString::fromUtf8(value);
        qDebug().noquote() << tagName << ":" << tagValue;
    }
}

