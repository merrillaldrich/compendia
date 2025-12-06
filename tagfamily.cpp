#include "tagfamily.h"

TagFamily::TagFamily(){
    tagFamilyName = "";
}

TagFamily::TagFamily(QString tf){
    tagFamilyName = tf;
}

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const TagFamily &t) {
    out << t.tagFamilyName;
    return out;
}

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, TagFamily &t) {
    in >> t.tagFamilyName;
    return in;
}
