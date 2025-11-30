#include "taggedfile.h"

TaggedFile::TaggedFile(QString fp, QString fn, QList<Tag*>* tl){
    filePath = fp;
    fileName = fn;
    tagList = tl;
}

