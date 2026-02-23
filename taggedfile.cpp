#include "taggedfile.h"


TaggedFile::TaggedFile(QObject *parent)
    : QObject{parent}{

}

TaggedFile::TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QMap<QString, QString>* exifMap, QObject *parent)
    : QObject{parent}{
    this->filePath = fileInfo.absolutePath();
    this->fileName = fileInfo.fileName();
    this->fileCreationDateTime = fileInfo.birthTime();
    this->fileModificationDateTime = fileInfo.lastModified();
    this->tags_ = tags;
}

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

QMap<QString, QString> TaggedFile::exifMap() const
{
    return exif_map_;
}

void TaggedFile::setExifMap(const QMap<QString, QString> &newExifMap)
{
    if (newExifMap != exif_map_){
        exif_map_ = newExifMap;
        dirty_flag_ = true;
    }
}

void TaggedFile::initExifMap(const QMap<QString, QString> &exifMap)
{
    exif_map_ = exifMap;
}

QSet<Tag *> *TaggedFile::tags()
{
    return tags_;
}

void TaggedFile::addTag(Tag* tag)
{
    tags_->insert(tag);
    dirty_flag_ = true;
}

void TaggedFile::removeTag(Tag* tag)
{
    tags_->remove(tag);
    dirty_flag_ = true;
}

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

void TaggedFile::markDirty()
{
    dirty_flag_ = true;
}

void TaggedFile::clearDirtyFlag()
{
    dirty_flag_ = false;
}
