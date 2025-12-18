#ifndef TAGGEDFILECOLLECTION_H
#define TAGGEDFILECOLLECTION_H

#include <QObject>
#include <QStandardItemModel>
#include <QPainter>
#include "tagfamily.h"
#include "tag.h"
#include "taggedfile.h"
#include "tagset.h"
#include "filterproxymodel.h"

class TaggedFileCollection : public QObject
{
    Q_OBJECT

private:
    QList<TagFamily*> *tag_families_;
    QList<Tag*> *tags_;

    //QList<TaggedFile*> *tagged_files_;
    QStandardItemModel *tagged_files_;
    FilterProxyModel *tagged_files_proxy_;

    QPixmap makeSquareIcon(const QPixmap &source, int size);

public:
    explicit TaggedFileCollection(QObject *parent = nullptr);

    bool containsFiles();

    QList<Tag*>* getLibraryTags();
    QList<Tag*>* getAssignedTags();

    Tag* getTag(QString tagFamilyName, QString tagName);
    TagFamily* getTagFamily(QString tagFamilyName);

    QList<TagSet> parseTagJson(QJsonObject tagsJson);

    void addFile(QString fp, QString fn);
    void addFile(QString fp, QString fn, QJsonObject tagsJson);
    void addFile(QString fp, QString fn, QList<TagSet> tags);

    void applyTag(Tag* droppedTag);
    void applyTag(TaggedFile* f, TagSet t);

    Tag* addLibraryTag(QString tagFamilyName, QString tagName);
    void addLibraryTag(Tag* tag);
    TagFamily* addLibraryTagFamily(QString tagFamilyName);
    void addLibraryTagFamily(TagFamily* tagFamily);

    void renameFamily(QString oldName, QString newName);

    QStandardItemModel* getItemModel();
    FilterProxyModel* getItemModelProxy();

    void setFileNameFilter(QString filterText);

};

#endif // TAGGEDFILECOLLECTION_H
