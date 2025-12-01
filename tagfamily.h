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

#endif // TAGFAMILY_H
