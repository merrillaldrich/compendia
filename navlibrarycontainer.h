#ifndef NAVLIBRARYCONTAINER_H
#define NAVLIBRARYCONTAINER_H

#include <QWidget>
#include <QLayout>
#include "tag.h"
#include "tagfamily.h"
#include "tagfamilywidget.h"

class NavLibraryContainer : public QWidget
{
    Q_OBJECT
public:
    explicit NavLibraryContainer(QWidget *parent = nullptr);
    void refresh(QList<Tag*>* tags);
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
};

#endif // NAVLIBRARYCONTAINER_H
