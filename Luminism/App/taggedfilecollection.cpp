#include "taggedfilecollection.h"
#include <QDebug>

TaggedFileCollection::TaggedFileCollection(QObject *parent)
    : QObject{parent}
{
    tag_family_collection_ = new QList<TagFamily>;
    tag_collection_ = new QList<Tag>;
    tagged_file_collection_ = new QList<TaggedFile>;

}

void TaggedFileCollection::addFile(QString f, QList<TagSet*> tags){
    // Add to to the Tagged Object collection and propagate its tags and tag families
    addTagFamilies(tags);

    // TODO: Make a new TaggedObject with the tags linked


    // Then add it
    //tagged_file_collection_.append(f);
}

void TaggedFileCollection::addTagFamilies(QList<TagSet*> tags){
    // For each item in the tag set add the family, if it doesn't already exist
    // and add the tag including a link to the family, if the tag doesn't already exist

    for(const TagSet *value : tags) {

        QString current_family = value->tagFamilyName;
        QString current_tag = value->tagName;

        qDebug() << current_family;
        qDebug() << current_tag;

        //if( ! tag_family_collection_->contains(current_family) ){
        //    tag_family_collection_->append(current_family);
        //}
        //
        //int index = tag_family_collection_->indexOf(current_family);
        //TagFamily tf = tag_family_collection_[index];

        //if( ! tag_collection_->contains(value.tagName)){
        //    Tag t = new Tag(tf, value.tagName);
        //    tag_collection_->append(t);
        //}
    }
}
