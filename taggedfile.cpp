#include "taggedfile.h"

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

    // Build exif object
    QJsonObject exifObj;
    for (auto it = exif_map_.cbegin(); it != exif_map_.cend(); ++it) {
        exifObj.insert(it.key(), it.value());
    }

    // Wrap both under top-level keys
    QJsonObject root;
    root.insert("tags", families);
    root.insert("exif", exifObj);

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

/*! \brief Removes a tag from this file and marks the file dirty.
 *
 * \param tag The Tag pointer to remove.
 */
void TaggedFile::removeTag(Tag* tag)
{
    tags_->remove(tag);
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
