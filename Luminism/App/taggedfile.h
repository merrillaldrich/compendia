#ifndef TAGGEDFILE_H
#define TAGGEDFILE_H

#include <QObject>
#include "tag.h"

class TaggedFile : public QObject
{
    Q_OBJECT

public:
    QString filePath;
    QString fileName;
    QList<Tag> tagList;

    explicit TaggedFile(QObject *parent = nullptr);
    TaggedFile(QString fp, QString fn, QList<Tag> tl);

signals:
};

#endif // TAGGEDFILE_H
