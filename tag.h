#ifndef TAG_H
#define TAG_H

#include <QDataStream>
#include "tagfamily.h"

class Tag
{

public:
    QString tagName;
    TagFamily* tagFamily;

    Tag();
    Tag(TagFamily* tf, QString t);

};

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const Tag &t);

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, Tag &t);

#endif // TAG_H
