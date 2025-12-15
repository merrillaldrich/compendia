#include "taggedfilecollection.h"
#include <QDebug>

TaggedFileCollection::TaggedFileCollection(QObject *parent)
    : QObject{parent}{
    tag_families_ = new QList<TagFamily*>();
    tags_ = new QList<Tag*>();

    //tagged_file_collection_ = new QList<TaggedFile*>();
    tagged_files_ = new QStandardItemModel();
}

QStandardItemModel* TaggedFileCollection::getItemModel(){
    return tagged_files_;
}

bool TaggedFileCollection::containsFiles(){
    //return ( tagged_file_collection_->count() > 0 );
    return ( tagged_files_->rowCount() > 0 );
}

QList<Tag*>* TaggedFileCollection::getLibraryTags(){
    return tags_;
}

Tag* TaggedFileCollection::getTag(QString tagFamilyName, QString tagName){
    Tag* matchingTag = nullptr;
    for( int i = 0; i < tags_->count(); ++i ){
        Tag* t = tags_->at(i);
        if ( t->tagName == tagName && t->tagFamily->tagFamilyName == tagFamilyName){
            matchingTag = t;
            break;
        }
    }
    return matchingTag;
}

TagFamily* TaggedFileCollection::getTagFamily(QString tagFamilyName){
    TagFamily* matchingTagFamily = nullptr;
    for( int i = 0; i < tag_families_->count(); ++i ){
        TagFamily* tf = tag_families_->at(i);
        if ( tf->tagFamilyName == tagFamilyName){
            matchingTagFamily = tf;
            break;
        }
    }
    return matchingTagFamily;
}

void TaggedFileCollection::applyTag(Tag* t){
    for( int i = 0; i < tagged_files_->rowCount(); ++i ){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        itemAsTaggedFile->tagList->append(t);
    }
}

void TaggedFileCollection::applyTag(TaggedFile* f, TagSet t){
    // TODO: deal with uniqueness of tags
    Tag* tag = getTag(t.tagFamilyName, t.tagName);
    f->tagList->append(tag);
}

Tag* TaggedFileCollection::addLibraryTag(QString tagFamilyName, QString tagName){

    TagFamily* tf = addLibraryTagFamily(tagFamilyName);
    Tag* matchingTag = nullptr;
    Tag* t;
    for(int i = 0; i < tags_->count(); ++i){
        t = tags_->at(i);
        if(t->tagName == tagName && t->tagFamily == tf){
            matchingTag = t;
            break;
        }
    }
    if(matchingTag == nullptr){
        matchingTag = new Tag(tf, tagName, this);
        tags_->append(matchingTag);
    }
    return matchingTag;
}

void TaggedFileCollection::addLibraryTag(Tag* tag){
    tags_->append(tag);
}


TagFamily* TaggedFileCollection::addLibraryTagFamily(QString tagFamilyName){

    TagFamily* matchingFam = nullptr;
    TagFamily* fam = nullptr;
    for(int i = 0; i < tag_families_->count(); ++i){
        fam = tag_families_->at(i);
        if(fam->tagFamilyName == tagFamilyName){
            matchingFam = fam;
            break;
        }
    }
    if(matchingFam == nullptr){
        matchingFam = new TagFamily(tagFamilyName, this);
        tag_families_->append(matchingFam);
    }
    return matchingFam;
}

void TaggedFileCollection::addLibraryTagFamily(TagFamily* tagFamily){
    if(! tag_families_->contains(tagFamily)){
        tag_families_->append(tagFamily);
    }
}

QList<TagSet> TaggedFileCollection::parseTagJson(QJsonObject tagsJson){
    QList<TagSet> tagSets;

    for (auto it = tagsJson.begin(); it != tagsJson.end(); ++it) {
        //qDebug() << "Key:" << it.key() << "Value:" << it.value();
        QJsonArray array = it.value().toArray();
        for (const QJsonValue &value : array){
            //qDebug() << "Tag:" << value.toString();
            tagSets.append(TagSet(it.key(),value.toString()));
        }
    }

    return tagSets;
}

void TaggedFileCollection::addFile(QString fp, QString fn){
    QList<TagSet> tagSets;
    addFile(fp,fn,tagSets);

}

void TaggedFileCollection::addFile(QString fp, QString fn, QJsonObject tagsJson){
    QList<TagSet> tagSets = parseTagJson(tagsJson);
    addFile(fp,fn,tagSets);
}

void TaggedFileCollection::addFile(QString fp, QString fn, QList<TagSet> tags){
    // Add to to the Tagged Object collection and collect links to its tags and tag families

    // For each item in the tag set add the family, if it doesn't already exist,
    // and add the tag including a link to the family, if the tag doesn't already exist

    QList<Tag*>* newOrExistingTags = new QList<Tag*>();

    for(const TagSet value : tags) {

        TagFamily *current_family = nullptr;

        for(TagFamily *fam : *tag_families_) {
            if (fam->tagFamilyName == value.tagFamilyName){
                // Found a match, use it
                current_family = fam;
            }
        }

        if( ! current_family ){
            current_family = new TagFamily(value.tagFamilyName, this);
            tag_families_->append(current_family);
        }

        Tag *current_tag = nullptr;

        for(Tag *tag : *tags_){
            if (tag->tagFamily->tagFamilyName == value.tagFamilyName && tag->tagName == value.tagName){
                current_tag = tag;
            }
        }

        if( ! current_tag ){
            current_tag = new Tag(current_family, value.tagName, this);
            tags_->append(current_tag);
        }

        //qDebug() << current_tag->tagFamily->tagFamilyName;
        //qDebug() << current_tag->tagName;

        newOrExistingTags->append(current_tag);

    }

    // Then add it
    TaggedFile *tf = new TaggedFile(fp, fn, newOrExistingTags);
    // Standard items have just an icon and text

    // Make an icon
    QPixmap p = QPixmap(fp + "/" + fn);
    QPixmap squarePixmap = makeSquareIcon(p, 100);

    QStandardItem *i = new QStandardItem(QIcon(squarePixmap), tf->fileName);
    // In order to store the custom object box it as a variant and store in item.setdata()
    i->setData(QVariant::fromValue(tf));
    tagged_files_->appendRow(i);
\
    // Debugging adding and retrieving variants

    //qDebug() << tagged_file_collection_->rowCount();

    //QStandardItem* item = tagged_file_collection_->item(tagged_file_collection_->rowCount()-1);
    //QVariant var = item->data();
    //TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

    //qDebug() << itemAsTaggedFile->fileName;
}

void TaggedFileCollection::renameFamily(QString oldName, QString newName){
    for(TagFamily *fam : *tag_families_) {
        if (fam->tagFamilyName == oldName){
            fam->tagFamilyName = newName;
        }
    }
}

QPixmap TaggedFileCollection::makeSquareIcon(const QPixmap &source, int size)
{
    // Scale the image to fit within a square, keeping aspect ratio
    QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Create a transparent square pixmap
    QPixmap square(size, size);
    square.fill(Qt::transparent);

    // Center the scaled image in the square
    QPainter painter(&square);
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.end();

    return square;
}


