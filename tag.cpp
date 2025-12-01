#include "tag.h"

Tag::Tag(TagFamily* tf, QString t){
    tagFamily = tf;
    tagName = t;
}

//bool Tag::operator==(const Tag* other) const noexcept {
//    return tagName == other->tagName && tagFamily->tagFamilyName == other->tagFamily->tagFamilyName;
//}
