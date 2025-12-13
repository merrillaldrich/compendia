#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include <QDir>
#include <QDebug>
#include <QStandardItemModel>
#include "taggedfilecollection.h"

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

    Tag* getTag(QString tagFamily, QString tag);
    Tag* addLibraryTag(QString tagFamily, QString tagName);
    TagFamily* addLibraryTagFamily(QString tagFamilyName);
    void applyTag(Tag* tag);
    void applyTag(TaggedFile* file, TagSet tagSet);

    QStandardItemModel* getItemModel();


public slots:

signals:
};

#endif // LUMINISMCORE_H
