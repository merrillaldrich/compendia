#ifndef PREVIEWCONTAINER_H
#define PREVIEWCONTAINER_H

#include <QColor>
#include <QList>
#include <QPair>
#include <QRectF>
#include <QSizeF>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
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

    /*! \brief Adds normalised bounding-rectangle overlays on top of the current image.
     *
     * Each rect is given as normalised coordinates in [0.0, 1.0] relative to the image
     * dimensions, paired with the colour to draw its outline.  Any previously set
     * overlays are removed and replaced.
     *
     * \param normalizedRects List of (normalised QRectF, QColor) pairs.
     */
    void setTagRects(const QList<QPair<QRectF, QColor>> &normalizedRects);

    /*! \brief Shows or hides the tag rectangle overlays without removing them from the scene.
     *
     * \param visible True to show overlays, false to hide them.
     */
    void setTagRectsVisible(bool visible);

signals:


private:
    void updateTimeLabel(qint64 position, qint64 duration);

    QGraphicsScene*    scene = nullptr;
    ZoomableGraphicsView* view = nullptr;
    QMediaPlayer*      mediaPlayer = nullptr;
    QAudioOutput*      audioOutput_ = nullptr;
    QGraphicsVideoItem* videoItem = nullptr;
    QList<QGraphicsItem*> tag_rect_items_;
    QSizeF             image_size_;
    bool               is_video_ = false;

    QWidget*           controlBar_ = nullptr;
    QPushButton*       playPauseButton_ = nullptr;
    QSlider*           positionSlider_ = nullptr;
    QLabel*            timeLabel_ = nullptr;
    bool               slider_being_dragged_ = false;

};

#endif // PREVIEWCONTAINER_H
