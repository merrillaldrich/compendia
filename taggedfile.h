#ifndef TAGGEDFILE_H
#define TAGGEDFILE_H

#include <QList>
#include "tag.h"

class TaggedFile
{

public:
    QString filePath;
    QString fileName;
    QList<Tag*>* tagList;

    TaggedFile(QString fp, QString fn, QList<Tag *> *tl);

};

#endif // TAGGEDFILE_H
