#include "taggedfile.h"

TaggedFile::TaggedFile(QObject *parent)
    : QObject{parent}{

}

TaggedFile::TaggedFile(QFileInfo fileInfo, QSet<Tag*>* tags, QObject *parent)
    : QObject{parent}{
    this->filePath = fileInfo.absolutePath();
    this->fileName = fileInfo.fileName();
    this->fileCreationDateTime = fileInfo.birthTime();
    this->fileModificationDateTime = fileInfo.lastModified();
    this->tags = tags;
}

QString TaggedFile::TaggedFileJSON(){

    // Return all the families and associated tags for a file

    QJsonObject families;
    Tag* t;
    QString fn;

    QSetIterator<Tag *> i(*tags);
    while (i.hasNext()) {
        Tag* t = i.next();
        fn = t->tagFamily->getName();
        if(!families.contains(fn)) {
            families.insert(fn, QJsonArray());
        }
        QJsonArray tags = families[fn].toArray();
        tags.append(t->getName());
        families[fn]=tags;
    }
    QJsonDocument doc(families);
    QString strJson(doc.toJson());
    return strJson;
}
