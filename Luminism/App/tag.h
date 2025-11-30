#ifndef TAG_H
#define TAG_H

#include <QObject>
#include "tagfamily.h"

class Tag : public QObject
{
    Q_OBJECT
public:
    QString tagName;
    TagFamily* tagFamily;

    explicit Tag(QObject *parent = nullptr);
    Tag(TagFamily *tf, QString t, QObject *parent = nullptr);

signals:
};

#endif // TAG_H
