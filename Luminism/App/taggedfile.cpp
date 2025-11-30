#include "taggedfile.h"

TaggedFile::TaggedFile(QObject *parent)
    : QObject{parent}{

}

TaggedFile::TaggedFile(QString fp, QString fn, QList<Tag> tl){
    filePath = fp;
    fileName = fn;
    tagList = tl;
}

