#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QDataStream>
#include "tagfamily.h"

class Tag : public QObject
{
    Q_OBJECT

public:
    explicit Tag(QObject *parent = nullptr);

    QString tagName;
    TagFamily* tagFamily;

    Tag(TagFamily* tf, QString t, QObject *parent = nullptr);

};

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const Tag &t);

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, Tag &t);

#endif // TAG_H
