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

/*! \brief Utility class for extracting EXIF metadata from JPEG and HEIF/HEIC image files.
 *
 * All public methods are static; an instance is not needed.  JPEG files are
 * handled via libexif, HEIF/HEIC files via libheif plus libexif for parsing
 * the embedded EXIF block.
 */
class ExifParser : public QObject
{
    Q_OBJECT

private:
    /*! \brief Loads raw EXIF data from a standard JPEG file using libexif.
     *
     * \param filePath Absolute path to the JPEG file.
     * \return A heap-allocated ExifData pointer, or nullptr on failure.
     */
    static ExifData* getExifStandard(QString filePath);

    /*! \brief Loads raw EXIF data from a HEIF/HEIC file using libheif then libexif.
     *
     * \param filePath Absolute path to the HEIC file.
     * \return A heap-allocated ExifData pointer, or nullptr on failure.
     */
    static ExifData* getExifHeif(QString filePath);

    /*! \brief Converts a libexif ExifData structure into a Qt key-value string map.
     *
     * \param ed The libexif data structure to iterate.
     * \return A map of EXIF tag names to their string values.
     */
    static QMap<QString, QString> exifTagsToMap(ExifData* ed);

public:
    /*! \brief Constructs an ExifParser (rarely needed; prefer the static interface).
     *
     * \param parent Optional Qt parent object.
     */
    explicit ExifParser(QObject *parent = nullptr);

    /*! \brief Returns a map of EXIF key-value pairs for the given image file.
     *
     * Dispatches to getExifStandard() for .jpg files and getExifHeif() for
     * .heic files; returns an empty map for unsupported formats or on error.
     *
     * \param filePath Absolute path to the image file.
     * \return A map of EXIF tag name strings to value strings.
     */
    static QMap<QString, QString> getExifMap(QString filePath);

signals:
};

#endif // EXIFPARSER_H
