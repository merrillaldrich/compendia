#ifndef TAGGEDFILECOLLECTION_H
#define TAGGEDFILECOLLECTION_H

#include <QStandardItemModel>
#include "tagfamily.h"
#include "tag.h"
#include "taggedfile.h"
#include "tagset.h"

class TaggedFileCollection
{

private:
    QList<TagFamily*> *tag_family_collection_;
    QList<Tag*> *tag_collection_;

    //QList<TaggedFile*> *tagged_file_collection_;
    QStandardItemModel *tagged_file_collection_;

public:
    explicit TaggedFileCollection();

    bool containsFiles();

    void addFile(QString fp, QString fn, QList<TagSet> tags);

    void renameFamily(QString oldName, QString newName);

};

#endif // TAGGEDFILECOLLECTION_H
