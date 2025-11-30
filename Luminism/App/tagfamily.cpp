#include "tagfamily.h"

TagFamily::TagFamily(QObject *parent)
    : QObject{parent}
{
    tagFamilyName = "";
}
TagFamily::TagFamily(QString tf, QObject *parent){
    tagFamilyName = tf;
}
