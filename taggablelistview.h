#ifndef TAGGABLELISTVIEW_H
#define TAGGABLELISTVIEW_H

#include <QListView>
#include <QDropEvent>
#include <QMimeData>
#include "mainwindow.h"
#include "taggedfile.h"
#include "tagset.h"

class TaggableListView : public QListView
{
public:
    explicit TaggableListView(QWidget *parent = nullptr);

protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

};

#endif // TAGGABLELISTVIEW_H
