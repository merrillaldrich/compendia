#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QDataStream>
#include "tagfamily.h"

class Tag : public QObject
{
    Q_OBJECT

private:
    QString tag_name_;

public:
    explicit Tag(QObject *parent = nullptr);

    TagFamily* tagFamily;

    Tag(TagFamily* tf, QString t, QObject *parent = nullptr);

    void setName(QString tagName);
    QString getName() const;

signals:
    void nameChanged();

};

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const Tag &t);

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, Tag &t);

#endif // TAG_H
