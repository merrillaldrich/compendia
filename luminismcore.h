#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include <QDir>
#include <QDebug>
#include <QStandardItemModel>
#include "taggedfilecollection.h"
#include "filterproxymodel.h"

class LuminismCore : public QObject
{
    Q_OBJECT
private:

    QString root_directory_;
    TaggedFileCollection *tfc;

public:

    explicit LuminismCore(QObject *parent = nullptr);

    void setRootDirectory(QString path);
    void loadRootDirectory();

    void writeFileMetadata();

    QList<Tag*>* getLibraryTags();
    QList<Tag*>* getAssignedTags();
    QList<Tag*>* getAssignedTags_FilteredFiles();
    Tag* getTag(QString tagFamilyName, QString tagName);
    TagFamily* getTagFamily(QString tagFamilyName);
    Tag* addLibraryTag(QString tagFamily, QString tagName);
    void addLibraryTag(Tag* tag);
    TagFamily* addLibraryTagFamily(QString tagFamilyName);
    void addLibraryTagFamily(TagFamily* tagFamily);
    void applyTag(Tag* tag);
    void applyTag(TaggedFile* file, TagSet tagSet);

    void setFileNameFilter(QString filterText);

    QStandardItemModel* getItemModel();
    FilterProxyModel* getItemModelProxy();

public slots:

signals:
};

#endif // LUMINISMCORE_H
