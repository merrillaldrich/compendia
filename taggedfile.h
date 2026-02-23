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

private:
    QMap<QString, QString> exif_map_;
    QSet<Tag*>* tags_ = new QSet<Tag*>;
    QMap<Tag*, QRect> tag_rects_;
    bool dirty_flag_ = false;


public:
    QString filePath = "";
    QString fileName = "";
    QDateTime imageCaptureDateTime;
    QDateTime fileCreationDateTime;
    QDateTime fileModificationDateTime;

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

    /*! \brief Adds a tag with a bounding rectangle to this file and marks the file dirty.
     *
     * \param tag  The Tag pointer to add.
     * \param rect The bounding rectangle within the image for this tag.
     */
    void addTag(Tag* tag, QRect rect);

    /*! \brief Sets or updates the bounding rectangle for an existing tag assignment and marks dirty.
     *
     * \param tag  The Tag pointer already applied to this file.
     * \param rect The new bounding rectangle.
     */
    void setTagRect(Tag* tag, QRect rect);

    /*! \brief Sets a bounding rectangle without marking the file dirty (used during load).
     *
     * \param tag  The Tag pointer already applied to this file.
     * \param rect The bounding rectangle to initialise from.
     */
    void initTagRect(Tag* tag, QRect rect);

    /*! \brief Returns the bounding rectangle for a tag assignment, if one is set.
     *
     * \param tag The Tag pointer to query.
     * \return The bounding rectangle, or std::nullopt if no rect is set.
     */
    std::optional<QRect> tagRect(Tag* tag) const;

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
};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
