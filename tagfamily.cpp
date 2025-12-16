#include "tagfamily.h"

TagFamily::TagFamily(QObject *parent)
    : QObject{parent}{
    tag_family_name_ = "";
}

TagFamily::TagFamily(QString tf, QObject *parent = nullptr){
    tag_family_name_ = tf;
}

void TagFamily::setName(QString tagFamilyName){
    if(tag_family_name_ != tagFamilyName){
        tag_family_name_ = tagFamilyName;
        emit nameChanged();
    }
}

QString TagFamily::getName() const {
    return tag_family_name_;
}

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const TagFamily &t) {
    QString name = t.getName();
    out << name;
    return out;
}

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, TagFamily &t) { 
    QString newName;
    in >> newName;
    t.setName(newName);
    return in;
}
