#ifndef TAGGEDFILE_H
#define TAGGEDFILE_H

#include <optional>
#include <QObject>
#include <QList>
#include <QMap>
#include <QRect>
#include <QString>
#include <QMetaType>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include "tag.h"

/*! \brief Holds all data associated with a single media file managed by Luminism.
 *
 * Stores the file path, timestamps, applied tags, and EXIF metadata for one
 * image file.  A dirty flag tracks whether unsaved changes exist, either on
 * the TaggedFile itself or on any of its applied Tags or their families.
 */
class TaggedFile : public QObject {

    Q_OBJECT

public:
    /*! \brief Distinguishes image files from video files. */
    enum MediaType { Image, Video };

private:
    QMap<QString, QString> exif_map_;
    QSet<Tag*>* tags_ = new QSet<Tag*>;
    QMap<Tag*, QRectF> tag_rects_;
    bool dirty_flag_ = false;
    quint64 pHash_ = 0;
    std::optional<int> rating_ = std::nullopt;
    MediaType mediaType_ = Image;
    qint64 fileSize_ = 0;


public:
    QString filePath = "";
    QString fileName = "";
    QDateTime imageCaptureDateTime;
    QDateTime fileCreationDateTime;
    QDateTime fileModificationDateTime;

    /*! \brief Date/time inferred from a timestamp embedded in the file name, if present.
     *
     * Populated at construction time by scanning the file's base name for an 8-digit
     * (YYYYMMDD) or 14-digit (YYYYMMDDHHmmss) numeric sequence whose first four digits
     * form a year in the range 2000–2999.  8-digit matches are given a noon (12:00:00)
     * time component.  Remains invalid when no qualifying sequence is found.
     */
    QDateTime fileNameInferredDate;

    /*! \brief Returns the best available date for this file.
     *
     * Priority: EXIF capture date → filename-inferred date → filesystem creation date.
     * Returns an invalid QDate when none of the three sources yields a valid date.
     */
    QDate effectiveDate() const;

    /*! \brief Constructs a default, empty TaggedFile.
     *
     * \param parent Optional Qt parent object.
     */
    TaggedFile(QObject *parent = nullptr);

    /*! \brief Constructs a TaggedFile from a QFileInfo, a tag set, and an EXIF map.
     *
     * \param fileInfo The file-system information for the media file.
     * \param tags     Pre-populated set of Tag pointers to assign.
     * \param exifMap  Initial EXIF key-value pairs (may be empty).
     * \param parent   Optional Qt parent object.
     */
    TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QMap<QString, QString>* exifMap, QObject *parent = nullptr);

    /*! \brief Serialises the file's tags and EXIF data to a JSON string.
     *
     * \return A UTF-8 JSON document string suitable for writing to a sidecar file.
     */
    QString TaggedFileJSON();

    /*! \brief Returns a copy of the EXIF map.
     *
     * \return The current EXIF key-value map.
     */
    QMap<QString, QString> exifMap() const;

    /*! \brief Replaces the EXIF map and marks the file dirty if the data changed.
     *
     * \param newExifMap The new EXIF key-value map.
     */
    void setExifMap(const QMap<QString, QString> &newExifMap);

    /*! \brief Sets the EXIF map without marking the file dirty (used during load).
     *
     * \param exifMap The EXIF key-value map to initialise from.
     */
    void initExifMap(const QMap<QString, QString> &exifMap);

    /*! \brief Returns a pointer to the set of tags currently applied to this file.
     *
     * \return Pointer to the internal tag set.
     */
    QSet<Tag *> *tags();

    /*! \brief Adds a tag to this file and marks the file dirty.
     *
     * \param tag The Tag pointer to add.
     */
    void addTag(Tag* tag);

    /*! \brief Adds a tag with a normalised bounding rectangle to this file and marks the file dirty.
     *
     * \param tag  The Tag pointer to add.
     * \param rect The bounding rectangle in normalised image coordinates (0.0–1.0).
     */
    void addTag(Tag* tag, QRectF rect);

    /*! \brief Sets or updates the normalised bounding rectangle for an existing tag assignment and marks dirty.
     *
     * \param tag  The Tag pointer already applied to this file.
     * \param rect The new bounding rectangle in normalised image coordinates (0.0–1.0).
     */
    void setTagRect(Tag* tag, QRectF rect);

    /*! \brief Sets a normalised bounding rectangle without marking the file dirty (used during load).
     *
     * \param tag  The Tag pointer already applied to this file.
     * \param rect The bounding rectangle in normalised image coordinates (0.0–1.0).
     */
    void initTagRect(Tag* tag, QRectF rect);

    /*! \brief Returns the normalised bounding rectangle for a tag assignment, if one is set.
     *
     * \param tag The Tag pointer to query.
     * \return The bounding rectangle in normalised image coordinates (0.0–1.0), or std::nullopt if not set.
     */
    std::optional<QRectF> tagRect(Tag* tag) const;

    /*! \brief Removes a tag from this file and marks the file dirty.
     *
     * \param tag The Tag pointer to remove.
     */
    void removeTag(Tag* tag);

    /*! \brief Returns true if this file has unsaved changes.
     *
     * Also returns true if any applied tag or its family has unsaved name changes.
     *
     * \return True if a save is needed.
     */
    bool dirtyFlag() const;

    /*! \brief Directly sets the dirty flag to true. */
    void markDirty();

    /*! \brief Clears the file-level dirty flag (does not clear tag or family flags). */
    void clearDirtyFlag();

    /*! \brief Returns whether this file is an image or a video. */
    MediaType mediaType() const { return mediaType_; }

    /*! \brief Returns the file size in bytes as recorded at load time. */
    qint64 fileSize() const { return fileSize_; }

    /*! \brief Returns the perceptual hash computed for this file (0 if not yet computed). */
    quint64 pHash() const;

    /*! \brief Sets the perceptual hash without marking the file dirty (used during JSON load). */
    void initPHash(quint64 hash);

    /*! \brief Sets the perceptual hash and marks the file dirty if the value changed.
     *
     * Used during backfill so newly computed hashes are persisted on next save.
     * \param hash The newly computed pHash value.
     */
    void setPHash(quint64 hash);

    /*! \brief Returns the user rating (1–5), or std::nullopt if no rating has been assigned. */
    std::optional<int> rating() const;

    /*! \brief Sets the rating without marking the file dirty (used during JSON load).
     *
     * \param rating A value in [1,5], or std::nullopt to clear.
     */
    void initRating(std::optional<int> rating);

    /*! \brief Sets the rating and marks the file dirty if the value changed.
     *
     * \param rating A value in [1,5], or std::nullopt to clear.
     */
    void setRating(std::optional<int> rating);
};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
