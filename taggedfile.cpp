#include "taggedfile.h"

#include <QRegularExpression>

/*! \brief Searches \p baseName for an embedded timestamp and converts it to a QDateTime.
 *
 * Looks for a run of exactly 14 or 8 consecutive digits that appears at the start of
 * \p baseName or immediately after a non-digit character, and whose first four digits
 * form a year in the range 2000–2999.  14-digit sequences are interpreted as
 * YYYYMMDDHHmmss; 8-digit sequences as YYYYMMDD with a noon (12:00:00) time component.
 * At each candidate position the 14-digit alternative is tested first, so a full
 * timestamp always wins over a date-only prefix.
 *
 * \param baseName File base name (without extension) to search.
 * \return A valid QDateTime on success; an invalid QDateTime when no match is found.
 */
static QDateTime inferDateTimeFromFileName(const QString &baseName)
{
    static const QRegularExpression re(
        R"((?:^|(?<=[^\d]))(\d{14}|\d{8})(?=[^\d]|$))");
    QRegularExpressionMatchIterator it = re.globalMatch(baseName);
    while (it.hasNext()) {
        QString seq = it.next().captured(1);
        int year = seq.left(4).toInt();
        if (year < 2000 || year > 2999)
            continue;
        if (seq.length() == 14) {
            QDateTime dt = QDateTime::fromString(seq, "yyyyMMddHHmmss");
            if (dt.isValid())
                return dt;
        } else {
            QDate d = QDate::fromString(seq, "yyyyMMdd");
            if (d.isValid())
                return QDateTime(d, QTime(12, 0, 0));
        }
    }
    return QDateTime();
}

/*! \brief Returns the best available date for this file.
 *
 * Priority: EXIF capture date → filename-inferred date → filesystem creation date.
 * Returns an invalid QDate when none of the three sources yields a valid date.
 */
QDate TaggedFile::effectiveDate() const
{
    if (imageCaptureDateTime.isValid())
        return imageCaptureDateTime.date();
    if (fileNameInferredDate.isValid())
        return fileNameInferredDate.date();
    return fileCreationDateTime.date();
}

/*! \brief Constructs a default, empty TaggedFile.
 *
 * \param parent Optional Qt parent object.
 */
TaggedFile::TaggedFile(QObject *parent)
    : QObject{parent}{

}

/*! \brief Constructs a TaggedFile from a QFileInfo, a tag set, and an EXIF map.
 *
 * \param fileInfo The file-system information for the media file.
 * \param tags     Pre-populated set of Tag pointers to assign.
 * \param exifMap  Initial EXIF key-value pairs (may be empty).
 * \param parent   Optional Qt parent object.
 */
TaggedFile::TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QMap<QString, QString>* exifMap, QObject *parent)
    : QObject{parent}{
    this->filePath = fileInfo.absolutePath();
    this->fileName = fileInfo.fileName();
    this->fileCreationDateTime = fileInfo.birthTime();
    this->fileModificationDateTime = fileInfo.lastModified();
    this->fileNameInferredDate = inferDateTimeFromFileName(fileInfo.completeBaseName());
    this->tags_ = tags;
}

/*! \brief Serialises the file's tags and EXIF data to a JSON string.
 *
 * \return A UTF-8 JSON document string suitable for writing to a sidecar file.
 */
QString TaggedFile::TaggedFileJSON(){

    // Build tags object
    QJsonObject families;
    QSetIterator<Tag *> i(*tags_);
    while (i.hasNext()) {
        Tag* t = i.next();
        QString fn = t->tagFamily->getName();
        if(!families.contains(fn)) {
            families.insert(fn, QJsonArray());
        }
        QJsonArray tags = families[fn].toArray();
        tags.append(t->getName());
        families[fn] = tags;
    }

    // Build tag_rects object (only non-empty entries)
    QJsonObject tagRectsObj;
    for (auto it = tag_rects_.cbegin(); it != tag_rects_.cend(); ++it) {
        Tag* t = it.key();
        const QRectF& r = it.value();
        QString fn = t->tagFamily->getName();
        QString tn = t->getName();
        QJsonObject familyRects = tagRectsObj[fn].toObject();
        QJsonArray rectArr;
        rectArr.append(r.x());
        rectArr.append(r.y());
        rectArr.append(r.width());
        rectArr.append(r.height());
        familyRects.insert(tn, rectArr);
        tagRectsObj[fn] = familyRects;
    }

    // Build exif object
    QJsonObject exifObj;
    for (auto it = exif_map_.cbegin(); it != exif_map_.cend(); ++it) {
        exifObj.insert(it.key(), it.value());
    }

    // Wrap under top-level keys
    QJsonObject root;
    root.insert("tags", families);
    if (!tagRectsObj.isEmpty())
        root.insert("tag_rects", tagRectsObj);
    root.insert("exif", exifObj);
    if (pHash_ != 0)
        root.insert("pHash", QString::number(pHash_, 16).rightJustified(16, '0'));

    QJsonDocument doc(root);
    return QString(doc.toJson());
}

/*! \brief Returns a copy of the EXIF map.
 *
 * \return The current EXIF key-value map.
 */
QMap<QString, QString> TaggedFile::exifMap() const
{
    return exif_map_;
}

/*! \brief Replaces the EXIF map and marks the file dirty if the data changed.
 *
 * \param newExifMap The new EXIF key-value map.
 */
void TaggedFile::setExifMap(const QMap<QString, QString> &newExifMap)
{
    if (newExifMap != exif_map_){
        exif_map_ = newExifMap;
        dirty_flag_ = true;
    }
}

/*! \brief Sets the EXIF map without marking the file dirty (used during load).
 *
 * \param exifMap The EXIF key-value map to initialise from.
 */
void TaggedFile::initExifMap(const QMap<QString, QString> &exifMap)
{
    exif_map_ = exifMap;
}

/*! \brief Returns a pointer to the set of tags currently applied to this file.
 *
 * \return Pointer to the internal tag set.
 */
QSet<Tag *> *TaggedFile::tags()
{
    return tags_;
}

/*! \brief Adds a tag to this file and marks the file dirty.
 *
 * \param tag The Tag pointer to add.
 */
void TaggedFile::addTag(Tag* tag)
{
    tags_->insert(tag);
    dirty_flag_ = true;
}

/*! \brief Adds a tag with a bounding rectangle to this file and marks the file dirty.
 *
 * \param tag  The Tag pointer to add.
 * \param rect The bounding rectangle within the image for this tag.
 */
void TaggedFile::addTag(Tag* tag, QRectF rect)
{
    tags_->insert(tag);
    tag_rects_.insert(tag, rect);
    dirty_flag_ = true;
}

/*! \brief Sets or updates the normalised bounding rectangle for an existing tag assignment and marks dirty.
 *
 * \param tag  The Tag pointer already applied to this file.
 * \param rect The new bounding rectangle in normalised image coordinates (0.0–1.0).
 */
void TaggedFile::setTagRect(Tag* tag, QRectF rect)
{
    tag_rects_.insert(tag, rect);
    dirty_flag_ = true;
}

/*! \brief Sets a normalised bounding rectangle without marking the file dirty (used during load).
 *
 * \param tag  The Tag pointer already applied to this file.
 * \param rect The bounding rectangle in normalised image coordinates (0.0–1.0).
 */
void TaggedFile::initTagRect(Tag* tag, QRectF rect)
{
    tag_rects_.insert(tag, rect);
}

/*! \brief Returns the normalised bounding rectangle for a tag assignment, if one is set.
 *
 * \param tag The Tag pointer to query.
 * \return The bounding rectangle in normalised image coordinates (0.0–1.0), or std::nullopt if not set.
 */
std::optional<QRectF> TaggedFile::tagRect(Tag* tag) const
{
    if (tag_rects_.contains(tag))
        return tag_rects_.value(tag);
    return std::nullopt;
}

/*! \brief Removes a tag from this file and marks the file dirty.
 *
 * \param tag The Tag pointer to remove.
 */
void TaggedFile::removeTag(Tag* tag)
{
    tags_->remove(tag);
    tag_rects_.remove(tag);
    dirty_flag_ = true;
}

/*! \brief Returns true if this file has unsaved changes.
 *
 * Also returns true if any applied tag or its family has unsaved name changes.
 *
 * \return True if a save is needed.
 */
bool TaggedFile::dirtyFlag() const
{
    if (dirty_flag_)
        return true;
    for (Tag* tag : *tags_) {
        if (tag->dirtyFlag() || tag->tagFamily->dirtyFlag())
            return true;
    }
    return false;
}

/*! \brief Directly sets the dirty flag to true. */
void TaggedFile::markDirty()
{
    dirty_flag_ = true;
}

/*! \brief Clears the file-level dirty flag (does not clear tag or family flags). */
void TaggedFile::clearDirtyFlag()
{
    dirty_flag_ = false;
}

/*! \brief Returns the perceptual hash computed for this file (0 if not yet computed). */
quint64 TaggedFile::pHash() const
{
    return pHash_;
}

/*! \brief Sets the perceptual hash without marking the file dirty (used during load and backfill). */
void TaggedFile::initPHash(quint64 hash)
{
    pHash_ = hash;
}
