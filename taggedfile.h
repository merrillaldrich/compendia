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
    QDateTime imageCaptureDateTime;
    QDateTime fileCreationDateTime;
    QDateTime fileModificationDateTime;
    QMap<QString, QString> exifMap;

    QSet<Tag*>* tags = new QSet<Tag*>;

    TaggedFile(QObject *parent = nullptr);
    TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QObject *parent = nullptr);

    QString TaggedFileJSON();

};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
