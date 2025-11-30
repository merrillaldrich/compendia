#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include "taggedfilecollection.h"

class LuminismCore : public QObject
{
    Q_OBJECT
public:
    explicit LuminismCore(QObject *parent = nullptr);

    TaggedFileCollection *tfc = new TaggedFileCollection();
signals:
};

#endif // LUMINISMCORE_H
