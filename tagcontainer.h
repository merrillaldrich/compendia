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

private:
    bool const famNameLessThan(const TagFamilyWidget &a, const TagFamilyWidget &b) ;

public:
    explicit TagContainer(QWidget *parent = nullptr);

    void refresh(QSet<Tag*>* tags);
    void clear();
    void onTagDeleteRequested(Tag* tag);
    void sort();

protected:

signals:
    void tagDeleteRequested(Tag* tag);
};

#endif // TAGCONTAINER_H
