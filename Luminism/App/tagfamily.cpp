#include "tagfamily.h"

TagFamily::TagFamily(){
    tagFamilyName = "";
}

TagFamily::TagFamily(QString tf){
    tagFamilyName = tf;
}

//bool TagFamily::operator==(const TagFamily other) const noexcept {
//    return tagFamilyName == other.tagFamilyName;
//}
//
//bool TagFamily::operator==(const QString other) const noexcept {
//    return tagFamilyName == other;
//}
