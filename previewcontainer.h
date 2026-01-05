#ifndef PREVIEWCONTAINER_H
#define PREVIEWCONTAINER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QImageReader>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QMediaPlayer>
#include <QMessageBox>

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
    QGraphicsView* view = nullptr;
    QMediaPlayer* mediaPlayer = nullptr;
    QGraphicsVideoItem* videoItem = nullptr;
    //QAbstractButton* playButton = nullptr;
    //QSlider* positionSlider = nullptr;

};

#endif // PREVIEWCONTAINER_H
