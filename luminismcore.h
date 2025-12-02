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
    explicit LuminismCore(QObject *parent = nullptr);

    TaggedFileCollection *tfc = new TaggedFileCollection();

    void setRootDirectory(QString path);
    void loadRootDirectory();

public slots:

signals:
};

#endif // LUMINISMCORE_H
