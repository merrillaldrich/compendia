#ifndef TAGGEDFILECOLLECTION_H
#define TAGGEDFILECOLLECTION_H

#include <QStandardItemModel>
#include <QPainter>
#include "tagfamily.h"
#include "tag.h"
#include "taggedfile.h"
#include "tagset.h"

class TaggedFileCollection
{

private:
    QList<TagFamily*> *tag_families_;
    QList<Tag*> *tags_;

    //QList<TaggedFile*> *tagged_files_;
    QStandardItemModel *tagged_files_;

    QPixmap makeSquareIcon(const QPixmap &source, int size);

public:
    explicit TaggedFileCollection();

    bool containsFiles();

    Tag* getTag(QString tagFamily, QString tag);

    void addFile(QString fp, QString fn, QList<TagSet> tags);

    void applyTag(Tag* droppedTag);

    void addLibraryTag(Tag* t);

    void renameFamily(QString oldName, QString newName);

    QStandardItemModel* getItemModel();

};

#endif // TAGGEDFILECOLLECTION_H
