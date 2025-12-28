#ifndef NAVLIBRARYCONTAINER_H
#define NAVLIBRARYCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "tag.h"
#include "tagfamily.h"
#include "tagfamilywidget.h"
#include "tagcontainer.h"

class NavLibraryContainer : public TagContainer
{
    Q_OBJECT
public:
    explicit NavLibraryContainer(QWidget *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
};

#endif // NAVLIBRARYCONTAINER_H
