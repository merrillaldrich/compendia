#ifndef TAGFAMILY_H
#define TAGFAMILY_H

#include <QString>

class TagFamily
{

public:
    QString tagFamilyName = "";

    TagFamily();
    TagFamily(QString tf);

//    bool operator==(const TagFamily other) const noexcept;
//    bool operator==(const QString other) const noexcept;

};

QDataStream &operator<<(QDataStream &out, const TagFamily &t);

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, TagFamily &t);


#endif // TAGFAMILY_H
