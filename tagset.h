#ifndef TAGSET_H
#define TAGSET_H

#include <QString>

class TagSet
{
public:
    QString tagFamilyName = "";
    QString tagName = "";

    TagSet(QString tfn, QString tn);

signals:
};

#endif // TAGSET_H
