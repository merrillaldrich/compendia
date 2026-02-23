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

/*! \brief Widget that displays a full-size image preview inside a zoomable graphics view.
 *
 * Wraps a ZoomableGraphicsView and a QGraphicsScene.  The preview can be loaded
 * from a file path or directly from a QImage.  The freshen() method re-fits the
 * image to the viewport when the containing pane is resized.
 */
class PreviewContainer : public QWidget
{
    Q_OBJECT
public:
    /*! \brief Constructs the preview container and sets up the internal scene and view.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit PreviewContainer(QWidget *parent = nullptr);

    /*! \brief Displays the given QImage in the preview pane.
     *
     * \param image The image to display.
     */
    void preview(QImage image);

    /*! \brief Loads and displays the image at the given absolute file path.
     *
     * \param absoluteFilePath Absolute path to the image file to display.
     */
    void preview(QString absoluteFilePath);

    /*! \brief Re-fits the displayed image to the viewport if it has not been zoomed in. */
    void freshen();

    /*! \brief Clears the graphics scene, removing any displayed image. */
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
