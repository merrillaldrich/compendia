#ifndef TAG_H
#define TAG_H

#include "tagfamily.h"

class Tag
{

public:
    QString tagName;
    TagFamily* tagFamily;

    Tag(TagFamily* tf, QString t);

//    bool operator==(const Tag *other) const noexcept;

};

#endif // TAG_H
