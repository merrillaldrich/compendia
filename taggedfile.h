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

private:
    QMap<QString, QString> exif_map_;
    QSet<Tag*>* tags_ = new QSet<Tag*>;
    bool dirty_flag_ = false;

public:
    QString filePath = "";
    QString fileName = "";
    QDateTime imageCaptureDateTime;
    QDateTime fileCreationDateTime;
    QDateTime fileModificationDateTime;


    TaggedFile(QObject *parent = nullptr);
    TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QMap<QString, QString>* exifMap, QObject *parent = nullptr);

    QString TaggedFileJSON();

    QMap<QString, QString> exifMap() const;
    void setExifMap(const QMap<QString, QString> &newExifMap);

    QSet<Tag *> *tags();
    bool dirtyFlag() const;
};

Q_DECLARE_METATYPE(TaggedFile)

#endif // TAGGEDFILE_H
