#ifndef TAGFAMILY_H
#define TAGFAMILY_H

#include <QObject>
#include <QString>

class TagFamily : public QObject
{
    Q_OBJECT

public:
    QString tagFamilyName = "";

    explicit TagFamily(QObject *parent = nullptr);
    TagFamily(QString tf, QObject *parent);

};

QDataStream &operator<<(QDataStream &out, const TagFamily &t);

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, TagFamily &t);


#endif // TAGFAMILY_H
