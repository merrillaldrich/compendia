#include "previewcontainer.h"
#include "constants.h"
#include <QPainter>
#include <QPen>
#include <QFileInfo>
#include <QUrl>

static bool isVideoFile(const QString &path)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    return videoExts.contains(QFileInfo(path).suffix().toLower());
}

// File-local item that draws a rounded rectangle with a cosmetic pen whose corner
// radius is expressed in screen pixels rather than scene units.  This keeps the
// rounding constant at any zoom level, matching the behaviour of the cosmetic pen.
class TagRectItem : public QGraphicsItem
{
    QRectF rect_;
    QPen   pen_;
public:
    TagRectItem(const QRectF &rect, const QPen &pen)
        : rect_(rect), pen_(pen) { setFlag(QGraphicsItem::ItemIsSelectable, false); }

    QRectF boundingRect() const override
    {
        // Cosmetic pens don't consume scene space; add 1-unit padding so Qt's
        // dirty-region tracking includes the stroke.
        return rect_.adjusted(-1, -1, 1, 1);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setPen(pen_);
        painter->setBrush(Qt::NoBrush);

        // Convert the fixed 4-screen-pixel corner radius to scene units using
        // the current horizontal scale of the world transform.
        const qreal screenRadius = 4.0;
        const qreal scaleX = painter->worldTransform().m11();
        const qreal cornerRadius = (scaleX > 0.0) ? screenRadius / scaleX : screenRadius;

        painter->drawRoundedRect(rect_, cornerRadius, cornerRadius);
    }
};

/*! \brief Constructs the preview container and sets up the internal scene and view.
 *
 * \param parent Optional Qt parent widget.
 */
PreviewContainer::PreviewContainer(QWidget *parent)
    : QWidget{parent}
{

    scene = new QGraphicsScene(this);
    view = new ZoomableGraphicsView(scene, this);
    view->setAcceptDrops(true);
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(view);

    connect(view, &ZoomableGraphicsView::tagDragEntered, this,
            &PreviewContainer::tagPreviewDragEntered);
    connect(view, &ZoomableGraphicsView::tagDragMoved, this,
            &PreviewContainer::updateDropPreviewRect);
    connect(view, &ZoomableGraphicsView::tagDragLeft, this,
            &PreviewContainer::clearDropPreviewRect);
    connect(view, &ZoomableGraphicsView::tagDropped, this,
            &PreviewContainer::handleTagDrop);

    mediaPlayer = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput_);

    // Control bar (hidden until a video is displayed)
    controlBar_ = new QWidget(this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar_);
    controlLayout->setContentsMargins(4, 2, 4, 2);

    playPauseButton_ = new QPushButton("Pause", controlBar_);
    playPauseButton_->setFixedWidth(60);
    controlLayout->addWidget(playPauseButton_);

    positionSlider_ = new QSlider(Qt::Horizontal, controlBar_);
    positionSlider_->setRange(0, 0);
    controlLayout->addWidget(positionSlider_);

    timeLabel_ = new QLabel("0:00 / 0:00", controlBar_);
    timeLabel_->setFixedWidth(90);
    timeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    controlLayout->addWidget(timeLabel_);

    layout->addWidget(controlBar_);
    controlBar_->hide();

    // Play/pause button toggles playback state
    connect(playPauseButton_, &QPushButton::clicked, this, [this]() {
        if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState)
            mediaPlayer->pause();
        else
            mediaPlayer->play();
    });

    // Keep button label in sync with actual playback state
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
        playPauseButton_->setText(state == QMediaPlayer::PlayingState ? "Pause" : "Play");
    });

    // When duration is known, configure slider range
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        positionSlider_->setMaximum(static_cast<int>(duration));
        updateTimeLabel(mediaPlayer->position(), duration);
    });

    // Advance slider as playback proceeds (suppressed while user is dragging)
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!slider_being_dragged_)
            positionSlider_->setValue(static_cast<int>(position));
        updateTimeLabel(position, mediaPlayer->duration());
    });

    // Track drag state to avoid fighting the player's position updates
    connect(positionSlider_, &QSlider::sliderPressed,   this, [this]() { slider_being_dragged_ = true; });
    connect(positionSlider_, &QSlider::sliderReleased,  this, [this]() {
        slider_being_dragged_ = false;
        mediaPlayer->setPosition(positionSlider_->value());
    });

    // Update time label in real time while scrubbing
    connect(positionSlider_, &QSlider::sliderMoved, this, [this](int value) {
        updateTimeLabel(value, mediaPlayer->duration());
    });
}

/*! \brief Updates the time label to show current position and total duration. */
void PreviewContainer::updateTimeLabel(qint64 position, qint64 duration)
{
    auto fmt = [](qint64 ms) -> QString {
        qint64 s = ms / 1000;
        return QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QChar('0'));
    };
    timeLabel_->setText(fmt(position) + " / " + fmt(duration));
}

/*! \brief Displays the given QImage in the preview pane.
 *
 * \param image The image to display.
 */
void PreviewContainer::preview(QImage image){
    mediaPlayer->stop();
    is_video_ = false;
    view->setAcceptDrops(true);
    controlBar_->hide();
    // Replace the image in the scene; scene->clear() deletes all items including overlays
    drop_preview_rect_ = nullptr; // scene->clear() below will delete it
    scene->clear();
    videoItem = nullptr; // scene->clear() deleted it if it was present
    tag_rect_items_.clear();
    image_size_ = image.size();

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene->addItem(item);
    scene->setSceneRect(scene->itemsBoundingRect()); // Prevents incorrect scrollbar extents

    item->setTransformationMode(Qt::SmoothTransformation);

    // Fix up zoom and such
    view->fitInView(item->boundingRect(), Qt::KeepAspectRatio);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->show();
}

/*! \brief Loads and displays the image at the given absolute file path.
 *
 * \param absoluteFilePath Absolute path to the image file to display.
 */
void PreviewContainer::preview(QString absoluteFilePath){

    if (isVideoFile(absoluteFilePath)) {
        mediaPlayer->stop();
        drop_preview_rect_ = nullptr; // scene->clear() below will delete it
        scene->clear();
        videoItem = nullptr;
        tag_rect_items_.clear();
        image_size_ = QSizeF();

        videoItem = new QGraphicsVideoItem;
        scene->addItem(videoItem);

        connect(videoItem, &QGraphicsVideoItem::nativeSizeChanged, this, [this](const QSizeF &size) {
            if (size.isEmpty()) return;
            videoItem->setSize(size);
            scene->setSceneRect(videoItem->boundingRect());
            view->fitInView(videoItem, Qt::KeepAspectRatio);
        });

        is_video_ = true;
        view->setAcceptDrops(false);
        controlBar_->show();
        mediaPlayer->setVideoSink(videoItem->videoSink());
        mediaPlayer->setSource(QUrl::fromLocalFile(absoluteFilePath));
        mediaPlayer->play();

        view->setDragMode(QGraphicsView::NoDrag);
        view->show();
        return;
    }

    QImageReader ir(absoluteFilePath);
    ir.setAutoTransform(true);

    // Load the image
    QImage image = ir.read();
    if (image.isNull()) {
        QMessageBox::critical(nullptr, "Error", "Failed to load image: " + ir.errorString());
    }

    preview(image);
}

/*! \brief Re-fits the displayed content to the viewport.
 *
 * For video, always re-fits so the playback fills the pane at any size.
 * For images, only re-fits when the image has not been zoomed in by the user.
 */
void PreviewContainer::freshen(){

    if (is_video_) {
        if (videoItem && !videoItem->boundingRect().isEmpty())
            view->fitInView(videoItem, Qt::KeepAspectRatio);
        return;
    }

    // Compare the size of the image to the viewport, and if the image is smaller
    // or equal then zoom extents to fill the viewport

    QRectF itemsRect = scene->itemsBoundingRect();
    QRectF viewportRect = view->mapToScene(view->viewport()->rect()).boundingRect();

    if (itemsRect.width() > viewportRect.width() + 30 ||
        itemsRect.height() > viewportRect.height() + 30) {
        // Image is zoomed in, do nothing
    } else {
        // Image is close in size to the viewport, zoom extents to "glue" the edges
        // of the image to the frame and resize with it
        view->fitInView(view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

}

/*! \brief Clears the graphics scene, removing any displayed image. */
void PreviewContainer::clear(){
    mediaPlayer->stop();
    is_video_ = false;
    view->setAcceptDrops(true);
    controlBar_->hide();
    positionSlider_->setValue(0);
    positionSlider_->setMaximum(0);
    timeLabel_->setText("0:00 / 0:00");
    if (view->scene() != nullptr){
        drop_preview_rect_ = nullptr; // scene->clear() below will delete it
        view->scene()->clear();
        videoItem = nullptr; // scene->clear() deleted it
        tag_rect_items_.clear();
        image_size_ = QSizeF();
    }
}

/*! \brief Adds normalised bounding-rectangle overlays on top of the current image.
 *
 * Each rect is given as normalised coordinates in [0.0, 1.0] relative to the image
 * dimensions, paired with the colour to draw its outline.  Any previously set
 * overlays are removed and replaced.
 *
 * \param normalizedRects List of (normalised QRectF, QColor) pairs.
 */
void PreviewContainer::setTagRects(const QList<QPair<QRectF, QColor>> &normalizedRects)
{
    // Delete existing overlay items (QGraphicsItem destructor removes itself from scene)
    for (QGraphicsItem* item : tag_rect_items_)
        delete item;
    tag_rect_items_.clear();

    if (image_size_.isEmpty()) return;

    for (const auto &[normRect, color] : normalizedRects) {
        QRectF sceneRect(
            normRect.x()      * image_size_.width(),
            normRect.y()      * image_size_.height(),
            normRect.width()  * image_size_.width(),
            normRect.height() * image_size_.height()
        );
        QPen pen(color);
        pen.setWidth(4);
        pen.setCosmetic(true);
        TagRectItem* item = new TagRectItem(sceneRect, pen);
        scene->addItem(item);
        tag_rect_items_.append(item);
    }
}

/*! \brief Shows or hides the tag rectangle overlays without removing them from the scene.
 *
 * \param visible True to show overlays, false to hide them.
 */
void PreviewContainer::setTagRectsVisible(bool visible)
{
    for (QGraphicsItem* item : tag_rect_items_)
        item->setVisible(visible);
}

/*! \brief Sets the colour used to draw the drop-preview rectangle during a tag drag.
 *
 * \param color The tag family colour for the tag currently being dragged.
 */
void PreviewContainer::setDropPreviewColor(QColor color)
{
    drop_preview_color_ = color;
}

/*! \brief Converts scenePos to a square clamped normalized rect and emits tagDroppedOnPreview.
 *
 * The rect is square in pixel space: its side length is DefaultTagRectSize multiplied by
 * the shorter image dimension.
 *
 * \param family   Tag family name.
 * \param tagName  Tag name.
 * \param scenePos Drop position in scene coordinates.
 */
void PreviewContainer::handleTagDrop(const QString &family,
                                     const QString &tagName,
                                     const QPointF &scenePos)
{
    if (image_size_.isEmpty() || is_video_) return;
    clearDropPreviewRect();
    qreal side_px = Luminism::DefaultTagRectSize * qMin(image_size_.width(), image_size_.height());
    qreal norm_w  = side_px / image_size_.width();
    qreal norm_h  = side_px / image_size_.height();
    QRectF rect(
        qBound(0.0, scenePos.x() / image_size_.width()  - norm_w / 2.0, 1.0 - norm_w),
        qBound(0.0, scenePos.y() / image_size_.height() - norm_h / 2.0, 1.0 - norm_h),
        norm_w, norm_h);
    emit tagDroppedOnPreview(family, tagName, rect);
}

/*! \brief Creates or repositions the drop-preview rectangle during a drag.
 *
 * Uses the same TagRectItem (rounded, cosmetic pen) as committed tag overlays, drawn in
 * the current drop_preview_color_.  The rect is square in pixel space.
 *
 * \param scenePos Current cursor position in scene coordinates.
 */
void PreviewContainer::updateDropPreviewRect(const QPointF &scenePos)
{
    if (image_size_.isEmpty()) return;
    qreal side_px = Luminism::DefaultTagRectSize * qMin(image_size_.width(), image_size_.height());
    qreal norm_w  = side_px / image_size_.width();
    qreal norm_h  = side_px / image_size_.height();
    QRectF sr(
        qBound(0.0, scenePos.x() / image_size_.width()  - norm_w / 2.0, 1.0 - norm_w) * image_size_.width(),
        qBound(0.0, scenePos.y() / image_size_.height() - norm_h / 2.0, 1.0 - norm_h) * image_size_.height(),
        side_px, side_px);
    // Delete and recreate so the new position is always in sync with the color
    if (drop_preview_rect_) {
        scene->removeItem(drop_preview_rect_);
        delete drop_preview_rect_;
    }
    QPen pen(drop_preview_color_, 4);
    pen.setCosmetic(true);
    drop_preview_rect_ = new TagRectItem(sr, pen);
    scene->addItem(drop_preview_rect_);
}

/*! \brief Removes and deletes the hover rectangle from the scene. */
void PreviewContainer::clearDropPreviewRect()
{
    if (drop_preview_rect_) {
        scene->removeItem(drop_preview_rect_);
        delete drop_preview_rect_;
        drop_preview_rect_ = nullptr;
    }
}
