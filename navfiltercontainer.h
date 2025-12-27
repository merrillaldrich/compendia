#ifndef NAVFILTERCONTAINER_H
#define NAVFILTERCONTAINER_H

#include <QWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLayout>
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "mainwindow.h"

class NavFilterContainer : public QWidget
{
    Q_OBJECT
public:
    explicit NavFilterContainer(QWidget *parent = nullptr);
    void refresh(QSet<Tag*>* tags);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void onTagDeleteRequested(Tag* tag);

signals:
    void tagDeleteRequested(Tag* tag);
};

#endif // NAVFILTERCONTAINER_H
