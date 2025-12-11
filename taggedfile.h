#ifndef TAGGEDFILE_H
#define TAGGEDFILE_H

#include <QList>
#include <QString>
#include <QMetaType>
#include <QJsonArray>
#include <QJsonObject>
#include "tag.h"

class TaggedFile {

public:
    QString filePath = "";
    QString fileName = "";
    QList<Tag*>* tagList = new QList<Tag*>;

    TaggedFile();

    TaggedFile(QString fp, QString fn, QList<Tag *> *tl);

    QString TaggedFileJSON();

};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
