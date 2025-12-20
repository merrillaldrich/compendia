#include "tag.h"

Tag::Tag(QObject *parent)
    : QObject{parent}{
    tagFamily = new TagFamily();
    tag_name_ = "";
}

Tag::Tag(TagFamily* tf, QString t, QObject *parent)
    : QObject{parent}{
    tagFamily = tf;
    tag_name_ = t;
}

void Tag::setName(QString tagName){
    if(tag_name_ != tagName){
        tag_name_ = tagName;
        emit nameChanged();
    }
}

QString Tag::getName() const {
    return tag_name_;
}

bool Tag::operator==(const Tag& other) const {
    return ( tag_name_ == other.getName() && tagFamily->getName() == other.tagFamily->getName() );
}

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const Tag &t) {
    QString name = t.getName();
    out << name;
    return out;
}

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, Tag &t) {
    QString newName;
    in >> newName;
    t.setName(newName);
    return in;
}
