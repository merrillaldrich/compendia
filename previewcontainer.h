#ifndef PREVIEWCONTAINER_H
#define PREVIEWCONTAINER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QImageReader>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>

#include <QMediaPlayer>
#include <QMessageBox>
#include "zoomablegraphicsview.h"

class PreviewContainer : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewContainer(QWidget *parent = nullptr);

    void preview(QImage image);
    void preview(QString absoluteFilePath);
    void freshen();
    void clear();

signals:


private:
    QGraphicsScene* scene = nullptr;
    ZoomableGraphicsView* view = nullptr;
    QMediaPlayer* mediaPlayer = nullptr;
    QGraphicsVideoItem* videoItem = nullptr;
    //QAbstractButton* playButton = nullptr;
    //QSlider* positionSlider = nullptr;

};

#endif // PREVIEWCONTAINER_H
