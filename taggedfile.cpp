#include "taggedfile.h"

TaggedFile::TaggedFile(QObject *parent)
    : QObject{parent}{

}

TaggedFile::TaggedFile(QFileInfo fileInfo,
                       QList<Tag*>* tl,
                       QObject *parent)
    : QObject{parent}{
    this->filePath = fileInfo.absolutePath();
    this->fileName = fileInfo.fileName();
    this->fileCreationDateTime = fileInfo.birthTime();
    this->fileModificationDateTime = fileInfo.lastModified();
    tagList = tl;
}

QString TaggedFile::TaggedFileJSON(){

    // Return all the families and associated tags for a file

    QJsonObject families;
    Tag* t;
    QString fn;

    for(int i = 0; i < tagList->count(); ++i){
        t = tagList->at(i);
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
