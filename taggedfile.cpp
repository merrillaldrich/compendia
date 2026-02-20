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

    // Return all the families and associated tags for a file

    QJsonObject families;
    Tag* t;
    QString fn;

    QSetIterator<Tag *> i(*tags_);
    while (i.hasNext()) {
        Tag* t = i.next();
        fn = t->tagFamily->getName();
        if(!families.contains(fn)) {
            families.insert(fn, QJsonArray());
        }
        QJsonArray tags = families[fn].toArray();
        tags.append(t->getName());
        families[fn]=tags;
    }
    QJsonDocument doc(families);
    QString strJson(doc.toJson());
    return strJson;
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
    return dirty_flag_;
}

void TaggedFile::markDirty()
{
    dirty_flag_ = true;
}

void TaggedFile::clearDirtyFlag()
{
    dirty_flag_ = false;
}
