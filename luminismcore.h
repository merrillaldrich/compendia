#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include <QDir>
#include <QDebug>
#include "taggedfilecollection.h"

class LuminismCore : public QObject
{
    Q_OBJECT
private:
    QString root_directory_;

public:
    TaggedFileCollection *tfc = new TaggedFileCollection();

    explicit LuminismCore(QObject *parent = nullptr);

    void setRootDirectory(QString path);
    void loadRootDirectory();
    void writeFileMetadata();

    Tag* getTag(QString tagFamily, QString tag);
    void addLibraryTag(Tag* t);
    void applyTag(Tag* droppedTag);

public slots:

signals:
};

#endif // LUMINISMCORE_H
