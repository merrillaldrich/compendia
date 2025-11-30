#ifndef TAGGEDFILECOLLECTION_H
#define TAGGEDFILECOLLECTION_H

#include <QObject>
#include "tagfamily.h"
#include "tag.h"
#include "taggedfile.h"
#include "tagset.h"

class TaggedFileCollection : public QObject
{
    Q_OBJECT

private:
    QList<TagFamily> *tag_family_collection_;
    QList<Tag> *tag_collection_;
    QList<TaggedFile> *tagged_file_collection_;

public:
    explicit TaggedFileCollection(QObject *parent = nullptr);

    void addFile(QString f, QList<TagSet *> tags);
    void addTagFamilies(QList<TagSet *> tags);

signals:
};

#endif // TAGGEDFILECOLLECTION_H
