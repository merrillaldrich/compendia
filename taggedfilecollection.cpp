#include "taggedfilecollection.h"
#include <QDebug>

TaggedFileCollection::TaggedFileCollection(QObject *parent)
    : QObject{parent}{
    tag_families_ = new QList<TagFamily*>();
    tags_ = new QList<Tag*>();

    //tagged_file_collection_ = new QList<TaggedFile*>();
    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);
}

QStandardItemModel* TaggedFileCollection::getItemModel(){
    return tagged_files_;
}

FilterProxyModel* TaggedFileCollection::getItemModelProxy(){
    return tagged_files_proxy_;
}

bool TaggedFileCollection::containsFiles(){
    //return ( tagged_file_collection_->count() > 0 );
    return ( tagged_files_->rowCount() > 0 );
}

QList<Tag*>* TaggedFileCollection::getLibraryTags(){
    return tags_;
}

QList<Tag*>* TaggedFileCollection::getAssignedTags(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    QList<Tag*>* distinctTags = new QList<Tag*>();

    for (int i = 0; i < tagged_files_->rowCount(); ++i){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        for (int j = 0; j < itemAsTaggedFile->tagList->count(); ++j){
            Tag* t = itemAsTaggedFile->tagList->at(j);
            if(!distinctTags->contains(t)){
                distinctTags->append(t);
            }
        }
    }
    return distinctTags;
}

QList<Tag*>* TaggedFileCollection::getAssignedTags_FilteredFiles(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    QList<Tag*>* distinctTags = new QList<Tag*>();

    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){

        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = getItemModelProxy()->mapToSource(proxyIndex);

        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

        for (int j = 0; j < itemAsTaggedFile->tagList->count(); ++j){
            Tag* t = itemAsTaggedFile->tagList->at(j);
            if(!distinctTags->contains(t)){
                distinctTags->append(t);
            }
        }
    }
    return distinctTags;
}

Tag* TaggedFileCollection::getTag(QString tagFamilyName, QString tagName){
    Tag* matchingTag = nullptr;
    for( int i = 0; i < tags_->count(); ++i ){
        Tag* t = tags_->at(i);
        if ( t->getName() == tagName && t->tagFamily->getName() == tagFamilyName){
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
        if ( tf->getName() == tagFamilyName){
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
        if(t->getName() == tagName && t->tagFamily == tf){
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
        if(fam->getName() == tagFamilyName){
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

void TaggedFileCollection::addFile(QFileInfo fileInfo){
    QList<TagSet> tagSets;
    addFile(fileInfo, tagSets);

}

void TaggedFileCollection::addFile(QFileInfo fileInfo, QJsonObject tagsJson){
    QList<TagSet> tagSets = parseTagJson(tagsJson);
    addFile(fileInfo, tagSets);
}

void TaggedFileCollection::addFile(QFileInfo fileInfo, QList<TagSet> tags){
    // Add to to the Tagged Object collection and collect links to its tags and tag families

    // For each item in the tag set add the family, if it doesn't already exist,
    // and add the tag including a link to the family, if the tag doesn't already exist

    QList<Tag*>* newOrExistingTags = new QList<Tag*>();

    for(const TagSet value : tags) {

        TagFamily *current_family = nullptr;

        for(TagFamily *fam : *tag_families_) {
            if (fam->getName() == value.tagFamilyName){
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
            if (tag->tagFamily->getName() == value.tagFamilyName && tag->getName() == value.tagName){
                current_tag = tag;
            }
        }

        if( ! current_tag ){
            current_tag = new Tag(current_family, value.tagName, this);
            tags_->append(current_tag);
        }

        newOrExistingTags->append(current_tag);
    }

    // Then add it
    TaggedFile *tf = new TaggedFile(fileInfo, newOrExistingTags);
    // Standard items have just an icon and text

    // Make an icon
    QPixmap p = QPixmap(tf->filePath + "/" + tf->fileName);
    QPixmap squarePixmap = makeSquareIcon(p, 100);

    QStandardItem *i = new QStandardItem(QIcon(squarePixmap), tf->fileName);
    // In order to store the custom object box it as a variant and store in item.setdata()
    i->setData(QVariant::fromValue(tf));
    tagged_files_->appendRow(i);
}

void TaggedFileCollection::renameFamily(QString oldName, QString newName){
    for(TagFamily *fam : *tag_families_) {
        if (fam->getName() == oldName){
            fam->setName(newName);
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

void TaggedFileCollection::setFileNameFilter(QString filterText){
    tagged_files_proxy_->setNameFilter(filterText);
}

