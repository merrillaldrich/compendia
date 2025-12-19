#ifndef TAGGEDFILE_H
#define TAGGEDFILE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QMetaType>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include "tag.h"

class TaggedFile : public QObject {

    Q_OBJECT

public:
    QString filePath = "";
    QString fileName = "";
    QDateTime fileCreationDateTime;
    QDateTime fileModificationDateTime;

    QList<Tag*>* tagList = new QList<Tag*>;

    TaggedFile(QObject *parent = nullptr);

    TaggedFile(QFileInfo fileInfo, QList<Tag *> *tl, QObject *parent = nullptr);

    QString TaggedFileJSON();

};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
