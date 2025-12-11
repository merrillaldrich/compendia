#include "taggedfile.h"

TaggedFile::TaggedFile(){

}

TaggedFile::TaggedFile(QString fp, QString fn, QList<Tag*>* tl){
    filePath = fp;
    fileName = fn;
    tagList = tl;
}

QString TaggedFile::TaggedFileJSON(){

    // Return all the families and associated tags for a file

    QJsonObject families;
    Tag* t;
    QString fn;

    for(int i = 0; i < tagList->count(); ++i){
        t = tagList->at(i);
        fn = t->tagFamily->tagFamilyName;
        if(!families.contains(fn)) {
            families.insert(fn, QJsonArray());
        }
        QJsonArray tags = families[fn].toArray();
        tags.append(t->tagName);
        families[fn]=tags;
    }
    QJsonDocument doc(families);
    QString strJson(doc.toJson());
    return strJson;
}
