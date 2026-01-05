#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QObject>
#include <QWheelEvent>
#include <QPointF>
#include <QScrollBar>


class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);
    ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;

};

#endif // ZOOMABLEGRAPHICSVIEW_H
