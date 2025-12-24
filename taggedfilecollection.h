#ifndef TAGGEDFILECOLLECTION_H
#define TAGGEDFILECOLLECTION_H

#include <QObject>
#include <QStandardItemModel>
#include <QPainter>
#include <QFileInfo>
#include <QtConcurrentRun>
#include "tagfamily.h"
#include "tag.h"
#include "taggedfile.h"
#include "tagset.h"
#include "filterproxymodel.h"
#include "icongenerator.h"

class TaggedFileCollection : public QObject
{
    Q_OBJECT

private:
    QSet<TagFamily*>* tag_families_;
    QSet<Tag*>* tags_;

    //QList<TaggedFile*> *tagged_files_;
    QStandardItemModel *tagged_files_;
    FilterProxyModel *tagged_files_proxy_;

    void onIconReady(const QString &absoluteFilePathName, const QPixmap &pixmap);

public:
    explicit TaggedFileCollection(QObject *parent = nullptr);

    bool containsFiles();

    QSet<Tag*>* getLibraryTags();
    QSet<Tag*>* getAssignedTags();
    QSet<Tag*>* getAssignedTags_FilteredFiles();

    Tag* getTag(QString tagFamilyName, QString tagName);
    TagFamily* getTagFamily(QString tagFamilyName);

    QList<TagSet> parseTagJson(QJsonObject tagsJson);

    void addFile(QFileInfo fileInfo);
    void addFile(QFileInfo fileInfo, QJsonObject tagsJson);
    void addFile(QFileInfo fileInfo, QList<TagSet> tags);
    void populateIcons();

    void applyTag(Tag* tag);
    void applyTag(TaggedFile* f, TagSet t);
    void unapplyTag(TaggedFile* file, Tag* tag);
    void unapplyTag(Tag* tag);

    Tag* addLibraryTag(QString tagFamilyName, QString tagName);
    void addLibraryTag(Tag* tag);
    TagFamily* addLibraryTagFamily(QString tagFamilyName);
    void addLibraryTagFamily(TagFamily* tagFamily);

    void renameFamily(QString oldName, QString newName);

    QStandardItemModel* getItemModel();
    FilterProxyModel* getItemModelProxy();

    void setFileNameFilter(QString filterText);

signals:
    void iconReady(const QString &absoluteFilePathName, const QPixmap &pixmap);

};

#endif // TAGGEDFILECOLLECTION_H
