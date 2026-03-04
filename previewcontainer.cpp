#include "previewcontainer.h"
#include "constants.h"
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QFontMetrics>
#include <QFileInfo>
#include <QUrl>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <memory>

static bool isVideoFile(const QString &path)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    return videoExts.contains(QFileInfo(path).suffix().toLower());
}

/*! \brief Graphics item that draws a rounded rectangle overlay and supports edge/corner dragging.
 *
 * When constructed with \p interactive = true the item accepts hover events and
 * left-button press/move/release events.  Hovering near any of the eight edge or
 * corner handles changes the cursor; dragging one of those handles resizes the rect
 * live.  On mouse release \c rectChanged is emitted with the new scene-coordinate rect.
 *
 * The corner radius and pen width are expressed in screen pixels (cosmetic), so they
 * remain constant at any zoom level.
 */
class TagRectItem : public QGraphicsObject
{
    Q_OBJECT

    // Hit-test margin in screen pixels.
    static constexpr qreal kHitPx   = 8.0;
    // Approximate folder-tab height reserved above the rect, in screen pixels.
    static constexpr qreal kTabPx   = 24.0;
    // Minimum rect width and height enforced during resize, in screen pixels.
    static constexpr qreal kMinPx   = 30.0;

    enum Handle { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight, Tab };

    QRectF  rect_;
    QPen    pen_;
    bool    interactive_;
    QString label_;
    Handle  active_handle_ = None;
    QPointF drag_start_scene_;
    QRectF  rect_at_drag_start_;

    /*! \brief Returns the folder-tab's bounding rect in scene/item coordinates. */
    QRectF tabSceneRect() const
    {
        qreal s = (scene() && !scene()->views().isEmpty())
                  ? scene()->views().first()->transform().m11() : 1.0;
        if (s <= 0.0) s = 1.0;
        QFont font;
        font.setPixelSize(11);
        const QFontMetrics fm(font);
        const qreal tabW = (fm.horizontalAdvance(label_) + 12) / s;  // 12 = 2 * hPad
        const qreal tabH = (fm.height() + 6) / s;                    //  6 = 2 * vPad
        return QRectF(rect_.left(), rect_.top() - tabH, tabW, tabH);
    }

    /*! \brief Returns the hit margin in scene units based on current view zoom. */
    qreal hitMargin() const
    {
        if (!scene() || scene()->views().isEmpty()) return kHitPx;
        qreal s = scene()->views().first()->transform().m11();
        return s > 0.0 ? kHitPx / s : kHitPx;
    }

    /*! \brief Returns which handle (if any) the local-coordinate point \p pos is over. */
    Handle hitTest(const QPointF &pos) const
    {
        qreal m = hitMargin();
        bool onL = qAbs(pos.x() - rect_.left())   < m;
        bool onR = qAbs(pos.x() - rect_.right())  < m;
        bool onT = qAbs(pos.y() - rect_.top())    < m;
        bool onB = qAbs(pos.y() - rect_.bottom()) < m;
        bool inH = pos.x() > rect_.left()  - m && pos.x() < rect_.right()  + m;
        bool inV = pos.y() > rect_.top()   - m && pos.y() < rect_.bottom() + m;
        if (onL && onT) return TopLeft;
        if (onR && onT) return TopRight;
        if (onL && onB) return BottomLeft;
        if (onR && onB) return BottomRight;
        if (onL && inV) return Left;
        if (onR && inV) return Right;
        if (onT && inH) return Top;
        if (onB && inH) return Bottom;
        if (!label_.isEmpty() && tabSceneRect().contains(pos)) return Tab;
        return None;
    }

    /*! \brief Maps a handle to the appropriate resize cursor shape. */
    static Qt::CursorShape cursorForHandle(Handle h)
    {
        switch (h) {
        case Left:  case Right:         return Qt::SizeHorCursor;
        case Top:   case Bottom:        return Qt::SizeVerCursor;
        case TopLeft: case BottomRight: return Qt::SizeFDiagCursor;
        case TopRight: case BottomLeft: return Qt::SizeBDiagCursor;
        case Tab:                       return Qt::SizeAllCursor;
        default:                        return Qt::ArrowCursor;
        }
    }

    /*! \brief Applies \p delta to the edge indicated by \p h, then normalizes the rect. */
    void applyHandle(Handle h, const QPointF &delta, QRectF &r) const
    {
        switch (h) {
        case Left:        r.setLeft(r.left()               + delta.x()); break;
        case Right:       r.setRight(r.right()             + delta.x()); break;
        case Top:         r.setTop(r.top()                 + delta.y()); break;
        case Bottom:      r.setBottom(r.bottom()           + delta.y()); break;
        case TopLeft:     r.setTopLeft(r.topLeft()         + delta);     break;
        case TopRight:    r.setTopRight(r.topRight()       + delta);     break;
        case BottomLeft:  r.setBottomLeft(r.bottomLeft()   + delta);     break;
        case BottomRight: r.setBottomRight(r.bottomRight() + delta);     break;
        case Tab:         r.translate(delta);                             break;
        default: break;
        }
    }

public:
    /*! \brief Constructs a TagRectItem.
     *
     * \param rect        Scene-coordinate bounding rect.
     * \param pen         Pen used to draw the outline (typically cosmetic).
     * \param interactive When true, hover and resize mouse events are enabled.
     * \param label       Optional tag name rendered in the folder-tab above the rect.
     */
    TagRectItem(const QRectF &rect, const QPen &pen, bool interactive = false,
                const QString &label = {})
        : rect_(rect), pen_(pen), interactive_(interactive), label_(label)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        if (interactive_)
            setAcceptHoverEvents(true);
    }

    /*! \brief Returns the bounding rect, expanded by the hit margin and the actual
     *  folder-tab geometry above and (when the label is wide) to the right. */
    QRectF boundingRect() const override
    {
        qreal m = interactive_ ? hitMargin() : 1.0;
        // Unite the main rect with the tab rect so the bounding box covers the
        // full painted area including any tab overhang to the right of rect_.
        return rect_.united(tabSceneRect()).adjusted(-m, -m, m, m);
    }

    /*! \brief Paints the rounded rectangle and the filled folder-tab above the top-left
     *  corner — everything drawn in screen (device) space so that the tab's left stroke
     *  and the rectangle's left stroke share the exact same pixel column at any zoom level. */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        // Capture all four corners in screen space before resetting the transform.
        const QTransform wt = painter->worldTransform();
        const qreal sl = wt.map(rect_.topLeft()).x();       // screen left
        const qreal st = wt.map(rect_.topLeft()).y();       // screen top
        const qreal sr = wt.map(rect_.bottomRight()).x();   // screen right
        const qreal sb = wt.map(rect_.bottomRight()).y();   // screen bottom

        const qreal cr   = 4.0;          // rect corner radius (screen px)
        const qreal tabR = cr * 0.75;    // tab corner radius (screen px)

        painter->save();
        painter->resetTransform();

        // Folder-tab geometry (constant screen-pixel size).
        const QColor fillColor = pen_.color();
        QFont font;
        font.setPixelSize(11);
        const QFontMetrics fm(font);
        const int hPad  = 6;
        const int vPad  = 3;
        const int textW = fm.horizontalAdvance(label_);
        const int tabW  = textW + 2 * hPad;
        const int tabH  = fm.height() + 2 * vPad;

        const qreal tabLeft   = sl;
        const qreal tabRight  = sl + tabW;
        const qreal tabBottom = st;   // tab bottom flush with rect top
        const qreal tabTop    = st - tabH;

        // 1. Tab fill — closed path, no stroke, drawn before the outline so the
        //    outline sits on top of the fill edge.
        QPainterPath tabFill;
        tabFill.moveTo(tabLeft, tabBottom);
        tabFill.lineTo(tabLeft, tabTop + tabR);
        tabFill.quadTo(tabLeft, tabTop, tabLeft + tabR, tabTop);
        tabFill.lineTo(tabRight - tabR, tabTop);
        tabFill.quadTo(tabRight, tabTop, tabRight, tabTop + tabR);
        tabFill.lineTo(tabRight, tabBottom);
        tabFill.closeSubpath();
        painter->fillPath(tabFill, QBrush(fillColor));

        // 2. Single unified outline — rect and tab as one closed path, one draw call.
        //    The left edge of the tab continues directly into the left edge of the
        //    rectangle (closeSubpath), so there is only one rasterised stroke for that
        //    edge and alignment is structurally guaranteed.
        //
        //    Winding: start at bottom-left of rect, go clockwise around the rect,
        //    detour up through the tab at the top-left, close back down the left edge.
        QPainterPath outline;
        outline.moveTo(sl, sb - cr);
        outline.quadTo(sl, sb,  sl + cr, sb);       // bottom-left corner
        outline.lineTo(sr - cr, sb);                // bottom edge
        outline.quadTo(sr, sb,  sr, sb - cr);       // bottom-right corner
        outline.lineTo(sr, st + cr);                // right edge
        outline.quadTo(sr, st,  sr - cr, st);       // top-right corner
        outline.lineTo(tabRight, st);               // rect top edge → tab
        outline.lineTo(tabRight, tabTop + tabR);    // tab right edge up
        outline.quadTo(tabRight, tabTop,  tabRight - tabR, tabTop);   // tab top-right
        outline.lineTo(tabLeft  + tabR, tabTop);    // tab top edge
        outline.quadTo(tabLeft,  tabTop, tabLeft,  tabTop + tabR);    // tab top-left
        outline.lineTo(sl, st);                     // tab left edge down to rect corner
        outline.closeSubpath();                     // rect left edge (sl) back to start

        // Second subpath: the rect top edge under the tab.  The outer contour skips
        // this segment, so it must be added explicitly to complete the visual line.
        outline.moveTo(tabLeft, tabBottom);
        outline.lineTo(tabRight, tabBottom);

        painter->setPen(pen_);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(outline);

        // 3. Label text — white on dark colours, black on light ones.
        const qreal luma = 0.299 * fillColor.redF()
                         + 0.587 * fillColor.greenF()
                         + 0.114 * fillColor.blueF();
        painter->setPen(luma > 0.5 ? Qt::black : Qt::white);
        painter->setFont(font);
        painter->drawText(QRectF(tabLeft + hPad, tabTop, textW, tabH), Qt::AlignVCenter, label_);

        painter->restore();
    }

    /*! \brief Updates the cursor to reflect which handle the cursor is over. */
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override
    {
        setCursor(cursorForHandle(hitTest(event->pos())));
        QGraphicsObject::hoverMoveEvent(event);
    }

    /*! \brief Restores the default cursor when the cursor leaves the item. */
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        unsetCursor();
        QGraphicsObject::hoverLeaveEvent(event);
    }

    /*! \brief Begins a resize drag if the press lands on an edge or corner handle. */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) { event->ignore(); return; }
        active_handle_ = hitTest(event->pos());
        if (active_handle_ != None) {
            drag_start_scene_  = event->scenePos();
            rect_at_drag_start_ = rect_;
            event->accept();
        } else {
            event->ignore();
        }
    }

    /*! \brief Updates the rect live while dragging a handle. */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (active_handle_ == None) { event->ignore(); return; }
        QPointF delta = event->scenePos() - drag_start_scene_;
        QRectF r = rect_at_drag_start_;
        applyHandle(active_handle_, delta, r);

        // Enforce minimum size: clamp whichever edge is moving so the rect
        // cannot shrink below kMinPx screen pixels or fold through the opposite edge.
        if (active_handle_ != Tab) {
            qreal s = (scene() && !scene()->views().isEmpty())
                      ? scene()->views().first()->transform().m11() : 1.0;
            if (s <= 0.0) s = 1.0;
            const qreal minScene = kMinPx / s;
            if (r.width() < minScene) {
                if (active_handle_ == Left || active_handle_ == TopLeft || active_handle_ == BottomLeft)
                    r.setLeft(r.right() - minScene);
                else
                    r.setRight(r.left() + minScene);
            }
            if (r.height() < minScene) {
                if (active_handle_ == Top || active_handle_ == TopLeft || active_handle_ == TopRight)
                    r.setTop(r.bottom() - minScene);
                else
                    r.setBottom(r.top() + minScene);
            }
        }

        prepareGeometryChange();
        rect_ = r;
        event->accept();
    }

    /*! \brief Commits the resize and emits rectChanged with the final scene rect. */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && active_handle_ != None) {
            active_handle_ = None;
            event->accept();
            emit rectChanged(rect_);
            return;
        }
        event->ignore();
    }

signals:
    /*! \brief Emitted when the user finishes resizing the rect.
     *  \param newSceneRect The final scene-coordinate rect after resizing.
     */
    void rectChanged(const QRectF &newSceneRect);
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
    tag_rect_descriptors_.clear();
    image_size_ = image.size();

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene->addItem(item);
    scene->setSceneRect(scene->itemsBoundingRect()); // Prevents incorrect scrollbar extents

    item->setTransformationMode(Qt::SmoothTransformation);

    // Fix up zoom and such
    view->fitInView(item->boundingRect(), Qt::KeepAspectRatio);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::NoDrag);
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
        tag_rect_descriptors_.clear();
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
        tag_rect_descriptors_.clear();
        image_size_ = QSizeF();
    }
}

/*! \brief Adds normalised bounding-rectangle overlays on top of the current image.
 *
 * Each descriptor carries a normalised rect, a colour for the outline and
 * folder-tab fill, and a label string rendered inside the tab.  Any previously
 * set overlays are removed and replaced.
 *
 * \param rects List of TagRectDescriptor values.
 */
void PreviewContainer::setTagRects(const QList<TagRectDescriptor> &rects)
{
    // Delete existing overlay items (QGraphicsItem destructor removes itself from scene)
    for (QGraphicsItem* item : tag_rect_items_)
        delete item;
    tag_rect_items_.clear();
    tag_rect_descriptors_.clear();

    if (image_size_.isEmpty()) return;

    tag_rect_descriptors_ = rects;

    for (const auto &desc : rects) {
        QRectF sceneRect(
            desc.rect.x()      * image_size_.width(),
            desc.rect.y()      * image_size_.height(),
            desc.rect.width()  * image_size_.width(),
            desc.rect.height() * image_size_.height()
        );
        QPen pen(desc.color);
        pen.setWidth(4);
        pen.setCosmetic(true);
        TagRectItem* item = new TagRectItem(sceneRect, pen, /*interactive=*/true, desc.label);
        scene->addItem(item);
        tag_rect_items_.append(item);

        // When the user finishes resizing, convert back to normalized coords and signal up.
        // normRectPtr is shared so the lambda can update it after each commit,
        // keeping oldNorm in sync for subsequent resizes on the same item.
        auto normRectPtr = std::make_shared<QRectF>(desc.rect);
        connect(item, &TagRectItem::rectChanged, this,
                [this, normRectPtr](const QRectF &newSceneRect) {
            if (image_size_.isEmpty()) return;
            QRectF newNorm(
                newSceneRect.x()      / image_size_.width(),
                newSceneRect.y()      / image_size_.height(),
                newSceneRect.width()  / image_size_.width(),
                newSceneRect.height() / image_size_.height());
            emit tagRectResized(*normRectPtr, newNorm);
            *normRectPtr = newNorm;
        });
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

/*! \brief Updates the stored normalized rect for one descriptor in-place.
 *
 * Called after a live resize so that the drop hit-test uses the current rect
 * without rebuilding the scene items.
 *
 * \param oldNorm The normalized rect to replace.
 * \param newNorm The new normalized rect.
 */
void PreviewContainer::updateTagRectDescriptor(const QRectF &oldNorm, const QRectF &newNorm)
{
    for (TagRectDescriptor &desc : tag_rect_descriptors_) {
        if (desc.rect == oldNorm) {
            desc.rect = newNorm;
            break;
        }
    }
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

    // If the drop lands inside an existing tag region, replace that tag rather
    // than creating a new region at the cursor position.
    QPointF normPos(scenePos.x() / image_size_.width(),
                    scenePos.y() / image_size_.height());
    for (const TagRectDescriptor &desc : tag_rect_descriptors_) {
        if (desc.rect.contains(normPos)) {
            emit tagDroppedOnExistingRegion(family, tagName, desc.rect);
            return;
        }
    }

    // Match the fixed screen-pixel size used by the drop-preview rectangle.
    static constexpr qreal kDropSideScreenPx = 60.0;
    qreal scale = view->transform().m11();
    if (scale <= 0.0) scale = 1.0;
    qreal side  = kDropSideScreenPx / scale;  // scene units
    qreal norm_w = side / image_size_.width();
    qreal norm_h = side / image_size_.height();
    QRectF rect(
        qBound(0.0, scenePos.x() / image_size_.width()  - norm_w / 2.0, 1.0 - norm_w),
        qBound(0.0, scenePos.y() / image_size_.height() - norm_h / 2.0, 1.0 - norm_h),
        norm_w, norm_h);
    emit tagDroppedOnPreview(family, tagName, rect);
}

/*! \brief Creates or repositions the drop-preview rectangle during a drag.
 *
 * Uses the same TagRectItem (rounded, cosmetic pen) as committed tag overlays, drawn in
 * the current drop_preview_color_.  The rect is a fixed size in screen pixels so it
 * remains the same visual size regardless of zoom level or image resolution.
 * The preview is hidden while the cursor is over an existing tag region.
 *
 * \param scenePos Current cursor position in scene coordinates.
 */
void PreviewContainer::updateDropPreviewRect(const QPointF &scenePos)
{
    if (image_size_.isEmpty()) return;

    // Hide the preview rect when hovering over an existing tag region.
    QPointF normPos(scenePos.x() / image_size_.width(),
                    scenePos.y() / image_size_.height());
    for (const TagRectDescriptor &desc : tag_rect_descriptors_) {
        if (desc.rect.contains(normPos)) {
            clearDropPreviewRect();
            return;
        }
    }

    // Fixed screen-pixel side length — independent of zoom and image resolution.
    static constexpr qreal kPreviewSideScreenPx = 60.0;
    qreal scale = view->transform().m11();
    if (scale <= 0.0) scale = 1.0;
    qreal side = kPreviewSideScreenPx / scale;  // convert to scene units

    QRectF sr(
        qBound(0.0, scenePos.x() - side / 2.0, image_size_.width()  - side),
        qBound(0.0, scenePos.y() - side / 2.0, image_size_.height() - side),
        side, side);

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

// Required so that moc processes TagRectItem (Q_OBJECT in a .cpp file).
#include "previewcontainer.moc"
