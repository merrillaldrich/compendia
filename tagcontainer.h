#ifndef TAGCONTAINER_H
#define TAGCONTAINER_H

#include <QObject>
#include <QWidget>
#include <QLayout>
#include <QSet>
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "tag.h"

class TagContainer : public QWidget
{
    Q_OBJECT
public:
    explicit TagContainer(QWidget *parent = nullptr);

    void refresh(QSet<Tag*>* tags);
    void onTagDeleteRequested(Tag* tag);

protected:

signals:
    void tagDeleteRequested(Tag* tag);
};

#endif // TAGCONTAINER_H
