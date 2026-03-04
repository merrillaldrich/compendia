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

/*! \brief Describes a single tag region overlay for display in the preview pane.
 *
 * Bundles the normalised bounding rectangle, the tag family colour, and the tag
 * name that will be rendered in the folder-tab label above the rectangle.
 */
struct TagRectDescriptor {
    QRectF  rect;   ///< Normalised bounding rectangle in [0, 1] × [0, 1].
    QColor  color;  ///< Tag family colour, used for the rectangle outline and tab fill.
    QString label;  ///< Tag name rendered inside the folder-tab above the rectangle.
};

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
     * Each descriptor carries a normalised rect, a colour for the outline and
     * folder-tab fill, and a label string rendered inside the tab.  Any previously
     * set overlays are removed and replaced.
     *
     * \param rects List of TagRectDescriptor values.
     */
    void setTagRects(const QList<TagRectDescriptor> &rects);

    /*! \brief Shows or hides the tag rectangle overlays without removing them from the scene.
     *
     * \param visible True to show overlays, false to hide them.
     */
    void setTagRectsVisible(bool visible);

    /*! \brief Updates the stored normalized rect for one descriptor in-place.
     *
     * Called after a live resize so that the drop hit-test uses the current rect
     * without rebuilding the scene items.
     *
     * \param oldNorm The normalized rect to replace.
     * \param newNorm The new normalized rect.
     */
    void updateTagRectDescriptor(const QRectF &oldNorm, const QRectF &newNorm);

    /*! \brief Sets the colour used to draw the drop-preview rectangle during a tag drag.
     *
     * \param color The tag family colour for the tag currently being dragged.
     */
    void setDropPreviewColor(QColor color);

signals:
    /*! \brief Emitted when a tag is dropped on the preview image.
     *  \param familyName     Tag family name decoded from the drag payload.
     *  \param tagName        Tag name decoded from the drag payload.
     *  \param normalizedRect Normalized (0–1) rect centred on the drop point.
     */
    void tagDroppedOnPreview(const QString &familyName,
                             const QString &tagName,
                             const QRectF  &normalizedRect);

    /*! \brief Emitted once when a tag drag first enters the preview view.
     *  \param family  Tag family name decoded from the drag payload.
     *  \param tagName Tag name decoded from the drag payload.
     */
    void tagPreviewDragEntered(const QString &family, const QString &tagName);

    /*! \brief Emitted when a tag is dropped directly onto an existing tag region.
     *
     * The drop target's normalized rect is passed so the receiver can identify
     * which tag currently owns that region and swap it for the dropped tag.
     *
     *  \param familyName    Tag family name of the dropped tag.
     *  \param tagName       Tag name of the dropped tag.
     *  \param existingRect  Normalized rect of the existing region that was hit.
     */
    void tagDroppedOnExistingRegion(const QString &familyName,
                                    const QString &tagName,
                                    const QRectF  &existingRect);

    /*! \brief Emitted when the user finishes resizing a tag region overlay.
     *  \param oldNormalizedRect The region's normalized rect before the resize.
     *  \param newNormalizedRect The region's normalized rect after the resize.
     */
    void tagRectResized(const QRectF &oldNormalizedRect, const QRectF &newNormalizedRect);

    /*! \brief Emitted when the user selects "Delete Tag" from a tag region's context menu.
     *  \param normalizedRect The normalized rect of the region to delete.
     */
    void tagRectDeleteRequested(const QRectF &normalizedRect);

private:
    /*! \brief Formats \a position and \a duration as "m:ss / m:ss" and updates the time label.
     *
     * \param position Current playback position in milliseconds.
     * \param duration  Total media duration in milliseconds.
     */
    void updateTimeLabel(qint64 position, qint64 duration);

    /*! \brief Converts scenePos to a clamped normalized rect and emits tagDroppedOnPreview.
     *
     * \param family   Tag family name.
     * \param tagName  Tag name.
     * \param scenePos Drop position in scene coordinates.
     */
    void handleTagDrop(const QString &family, const QString &tagName,
                       const QPointF &scenePos);

    /*! \brief Creates or repositions the hover rectangle during a drag.
     *
     * \param scenePos Current cursor position in scene coordinates.
     */
    void updateDropPreviewRect(const QPointF &scenePos);

    /*! \brief Removes and deletes the hover rectangle from the scene. */
    void clearDropPreviewRect();

    QGraphicsScene*    scene = nullptr;
    ZoomableGraphicsView* view = nullptr;
    QMediaPlayer*      mediaPlayer = nullptr;
    QAudioOutput*      audioOutput_ = nullptr;
    QGraphicsVideoItem* videoItem = nullptr;
    QList<QGraphicsItem*>    tag_rect_items_;
    QList<TagRectDescriptor> tag_rect_descriptors_;
    QGraphicsItem*     drop_preview_rect_ = nullptr;
    QColor             drop_preview_color_ = Qt::white;
    QSizeF             image_size_;
    bool               is_video_ = false;

    QWidget*           controlBar_ = nullptr;
    QPushButton*       playPauseButton_ = nullptr;
    QSlider*           positionSlider_ = nullptr;
    QLabel*            timeLabel_ = nullptr;
    bool               slider_being_dragged_ = false;

};

#endif // PREVIEWCONTAINER_H
