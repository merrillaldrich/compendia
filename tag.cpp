#include "tag.h"

Tag::Tag(){
    tagFamily = new TagFamily();
    tagName = "";
}

Tag::Tag(TagFamily* tf, QString t){
    tagFamily = tf;
    tagName = t;
}


// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const Tag &t) {
    //out << t.tagFamily << t.tagName;
    out << t.tagName;
    return out;
}

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, Tag &t) {
    //in >> t.tagFamily >> t.tagName;
    in >> t.tagName;
    return in;
}
