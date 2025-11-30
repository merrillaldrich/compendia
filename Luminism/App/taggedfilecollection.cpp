#include "taggedfilecollection.h"
#include <QDebug>

TaggedFileCollection::TaggedFileCollection(){
    tag_family_collection_ = new QList<TagFamily*>();
    tag_collection_ = new QList<Tag*>();
    tagged_file_collection_ = new QList<TaggedFile*>();
}

void TaggedFileCollection::addFile(QString fp, QString fn, QList<TagSet> tags){
    // Add to to the Tagged Object collection and collect links to its tags and tag families

    // For each item in the tag set add the family, if it doesn't already exist,
    // and add the tag including a link to the family, if the tag doesn't already exist

    QList<Tag*>* newOrExistingTags = new QList<Tag*>();

    for(const TagSet value : tags) {

        TagFamily *current_family = nullptr;

        for(TagFamily *fam : *tag_family_collection_) {
            if (fam->tagFamilyName == value.tagFamilyName){
                // Found a match, use it
                current_family = fam;
            }
        }

        if( ! current_family ){
            current_family = new TagFamily(value.tagFamilyName);
            tag_family_collection_->append(current_family);
        }

        Tag *current_tag = nullptr;

        for(Tag *tag : *tag_collection_){
            if (tag->tagFamily->tagFamilyName == value.tagFamilyName && tag->tagName == value.tagName){
                current_tag = tag;
            }
        }

        if( ! current_tag ){
            current_tag = new Tag(current_family, value.tagName);
            tag_collection_->append(current_tag);
        }

        qDebug() << current_tag->tagFamily->tagFamilyName;
        qDebug() << current_tag->tagName;

        newOrExistingTags->append(current_tag);

    }

    // Then add it
    TaggedFile *tf = new TaggedFile(fp, fn, newOrExistingTags);
    tagged_file_collection_->append(tf);
}

void TaggedFileCollection::renameFamily(QString oldName, QString newName){
    for(TagFamily *fam : *tag_family_collection_) {
        if (fam->tagFamilyName == oldName){
            fam->tagFamilyName = newName;
        }
    }
}

