#include "previewcontainer.h"
#include "constants.h"
#include "exifparser.h"
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QAbstractAnimation>
#include <QCursor>
#include <QEvent>
#include <QFontMetrics>
#include <QFileInfo>
#include <QMouseEvent>
#include <QUrl>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <memory>

static bool isVideoFile(const QString &path)
{
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    return videoExts.contains(QFileInfo(path).suffix().toLower());
}

// Horizontal distance from the view edge that triggers the nav button fade-in.
static constexpr int kNavHotZoneWidth = 100;

/*! \brief Circular overlay button that draws a left or right chevron arrow.
 *
 * Draws a semi-transparent dark circle with a white chevron.  Does not steal
 * keyboard focus when clicked.  The circle and arrow are rendered entirely in
 * paintEvent so no stylesheet or platform style painting occurs.
 */
class NavArrowButton : public QAbstractButton
{
    bool leftArrow_;
public:
    /*! \brief Constructs a NavArrowButton.
     *
     * \param leftArrow True for a left-pointing chevron, false for right.
     * \param parent    Optional Qt parent widget.
     */
    explicit NavArrowButton(bool leftArrow, QWidget *parent = nullptr)
        : QAbstractButton(parent), leftArrow_(leftArrow)
    {
        setFixedSize(48, 48);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    /*! \brief Paints a semi-transparent circle with a white chevron arrow. */
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Semi-transparent dark circle
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 150));
        p.drawEllipse(rect().adjusted(2, 2, -2, -2));

        // White chevron
        const QRectF r = rect();
        const qreal cx = r.center().x(), cy = r.center().y();
        const qreal aw = 9.0, ah = 9.0;
        QPainterPath path;
        if (leftArrow_) {
            path.moveTo(cx + aw * 0.5, cy - ah);
            path.lineTo(cx - aw * 0.5, cy);
            path.lineTo(cx + aw * 0.5, cy + ah);
        } else {
            path.moveTo(cx - aw * 0.5, cy - ah);
            path.lineTo(cx + aw * 0.5, cy);
            path.lineTo(cx - aw * 0.5, cy + ah);
        }
        p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
};

/*! \brief Control-bar button that draws a speaker icon; re-draws when mute state changes.
 *
 * Paints a filled trapezoid (speaker body + cone) and two arc sound waves.
 * In muted state the waves are replaced by a red cross.
 */
class SpeakerButton : public QAbstractButton
{
    bool muted_ = true;
public:
    /*! \brief Constructs a SpeakerButton.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit SpeakerButton(QWidget *parent = nullptr)
        : QAbstractButton(parent)
    {
        setFixedSize(28, 24);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
    }

    /*! \brief Updates the muted state and schedules a repaint.
     *
     * \param muted True to show the muted (crossed-out) icon.
     */
    void setMuted(bool muted) { muted_ = muted; update(); }

protected:
    /*! \brief Paints the speaker icon in the current mute state. */
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Speaker shape: rectangle body (left) + flaring cone (right), Windows 11 style
        // Body: x=3..8, y=9..15 (6px tall). Cone: x=8..13, flares to y=5..19 (14px tall).
        QPainterPath speaker;
        speaker.moveTo(3,  9);   // top-left of body
        speaker.lineTo(8,  9);   // top-right of body / junction
        speaker.lineTo(13, 5);   // top-right of cone (flared)
        speaker.lineTo(13, 19);  // bottom-right of cone (flared)
        speaker.lineTo(8,  15);  // bottom-right of body / junction
        speaker.lineTo(3,  15);  // bottom-left of body
        speaker.closeSubpath();
        static const QColor kIconGray(200, 200, 200);

        p.fillPath(speaker, kIconGray);

        if (!muted_) {
            p.setPen(QPen(kIconGray, 1.5, Qt::SolidLine, Qt::RoundCap));
            p.setBrush(Qt::NoBrush);
            // Inner wave arc
            p.drawArc(QRectF(12, 7, 6, 10), -60 * 16, 120 * 16);
            // Outer wave arc
            p.drawArc(QRectF(12, 4, 11, 16), -60 * 16, 120 * 16);
        } else {
            // Muted: cross in the waves area (30% smaller, same center at 19.5, 12)
            p.setPen(QPen(kIconGray, 2.0, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(16.35, 8.5),  QPointF(22.65, 15.5));
            p.drawLine(QPointF(22.65, 8.5),  QPointF(16.35, 15.5));
        }
    }
};

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

    /*! \brief Shows the right-click context menu for the tag region. */
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        if (!interactive_) { event->ignore(); return; }
        QMenu menu;
        QAction *findPersonAct = menu.addAction(QObject::tr("Find this person"));
        menu.addSeparator();
        QAction *deleteAct = menu.addAction(QObject::tr("Delete Tag"));
        QAction *chosen = menu.exec(event->screenPos());
        if (chosen == deleteAct)
            emit deleteRequested(rect_);
        else if (chosen == findPersonAct)
            emit findPersonRequested(rect_);
        event->accept();
    }

signals:
    /*! \brief Emitted when the user finishes resizing the rect.
     *  \param newSceneRect The final scene-coordinate rect after resizing.
     */
    void rectChanged(const QRectF &newSceneRect);

    /*! \brief Emitted when the user selects "Delete Tag" from the context menu.
     *  \param sceneRect The scene-coordinate rect of this item.
     */
    void deleteRequested(const QRectF &sceneRect);

    /*! \brief Emitted when the user selects "Find this person" from the context menu.
     *  \param sceneRect The scene-coordinate rect of this item.
     */
    void findPersonRequested(const QRectF &sceneRect);
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

    auto *speakerBtn = new SpeakerButton(controlBar_);
    volumeButton_ = speakerBtn;
    controlLayout->addWidget(volumeButton_);

    timeLabel_ = new QLabel("0:00 / 0:00", controlBar_);
    timeLabel_->setFixedWidth(90);
    timeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    controlLayout->addWidget(timeLabel_);

    layout->addWidget(controlBar_);
    controlBar_->hide();

    // Default audio state: muted, volume pre-set to 80% for when the user unmutes
    audioOutput_->setMuted(true);
    audioOutput_->setVolume(0.8f);

    // Volume popup — a frameless popup that auto-dismisses on outside click
    volumePopup_ = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint);
    volumePopup_->setFrameShape(QFrame::StyledPanel);
    volumePopup_->setFrameShadow(QFrame::Raised);
    auto *popupLayout = new QVBoxLayout(volumePopup_);
    popupLayout->setContentsMargins(8, 8, 8, 8);
    popupLayout->setSpacing(6);

    volumeSlider_ = new QSlider(Qt::Vertical, volumePopup_);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(80);
    volumeSlider_->setFixedHeight(100);
    popupLayout->addWidget(volumeSlider_, 0, Qt::AlignHCenter);

    auto *muteBtn = new SpeakerButton(volumePopup_);
    muteBtn->setCheckable(true);
    muteBtn->setChecked(true);
    muteBtn->setMuted(true);   // default: muted
    muteButton_ = muteBtn;
    popupLayout->addWidget(muteButton_, 0, Qt::AlignHCenter);

    volumePopup_->hide();

    // Show/hide the popup when the speaker button is clicked
    connect(volumeButton_, &QAbstractButton::clicked, this, [this]() {
        if (volumePopup_->isVisible()) {
            volumePopup_->hide();
            return;
        }
        volumePopup_->adjustSize();
        const QSize ps = volumePopup_->sizeHint();
        const QPoint globalPos = volumeButton_->mapToGlobal(
            QPoint(volumeButton_->width() / 2 - ps.width() / 2, -ps.height() - 4));
        volumePopup_->move(globalPos);
        volumePopup_->show();
    });

    // Mute toggle updates audio output and both speaker icons
    connect(muteButton_, &QAbstractButton::toggled, this, [this, speakerBtn, muteBtn](bool checked) {
        audioOutput_->setMuted(checked);
        speakerBtn->setMuted(checked);
        muteBtn->setMuted(checked);
    });

    // Volume slider updates audio output level
    connect(volumeSlider_, &QSlider::valueChanged, this, [this](int value) {
        audioOutput_->setVolume(value / 100.0f);
    });

    // Navigation overlay buttons (positioned manually over the view, not in the layout)
    navLeftButton_  = new NavArrowButton(true,  this);
    navRightButton_ = new NavArrowButton(false, this);

    navLeftOpacity_  = new QGraphicsOpacityEffect(navLeftButton_);
    navRightOpacity_ = new QGraphicsOpacityEffect(navRightButton_);
    navLeftOpacity_->setOpacity(0.0);
    navRightOpacity_->setOpacity(0.0);
    navLeftButton_->setGraphicsEffect(navLeftOpacity_);
    navRightButton_->setGraphicsEffect(navRightOpacity_);

    navFadeAnim_ = new QVariantAnimation(this);
    navFadeAnim_->setDuration(180);
    navFadeAnim_->setEasingCurve(QEasingCurve::InOutQuad);
    connect(navFadeAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        const qreal opacity = v.toReal();
        navLeftOpacity_->setOpacity(opacity);
        navRightOpacity_->setOpacity(opacity);
    });

    connect(navLeftButton_,  &QAbstractButton::clicked, this, [this]{ emit navigateRequested(-1); });
    connect(navRightButton_, &QAbstractButton::clicked, this, [this]{ emit navigateRequested(+1); });

    view->setMouseTracking(true);
    view->installEventFilter(this);
    navLeftButton_->installEventFilter(this);
    navRightButton_->installEventFilter(this);

    // Play/pause button toggles playback state.
    // If a video thumbnail is shown but playback hasn't started yet, clicking
    // Play sets up the videoItem and starts from the beginning of the file.
    connect(playPauseButton_, &QPushButton::clicked, this, [this]() {
        if (!pending_video_path_.isEmpty()) {
            // Transition from static thumbnail to real video playback.
            drop_preview_rect_ = nullptr;
            scene->clear();
            videoItem = nullptr;

            videoItem = new QGraphicsVideoItem;
            scene->addItem(videoItem);
            connect(videoItem, &QGraphicsVideoItem::nativeSizeChanged, this, [this](const QSizeF &size) {
                if (size.isEmpty()) return;
                videoItem->setSize(size);
                scene->setSceneRect(videoItem->boundingRect());
                view->fitInView(videoItem, Qt::KeepAspectRatio);
            });

            mediaPlayer->setVideoSink(videoItem->videoSink());
            mediaPlayer->setSource(QUrl::fromLocalFile(pending_video_path_));
            pending_video_path_.clear();
            mediaPlayer->play();
        } else if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            mediaPlayer->pause();
        } else {
            mediaPlayer->play();
        }
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
    if (frameSink_) {
        frameSink_->disconnect();
        frameSink_->deleteLater();
        frameSink_ = nullptr;
    }
    pending_video_path_.clear();
    mediaPlayer->stop();
    is_video_ = false;
    view->setAcceptDrops(true);
    controlBar_->hide();
    if (volumePopup_) volumePopup_->hide();
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

    // Always cancel any in-progress frame capture before doing anything else.
    // This must run unconditionally so that switching from a video to an image
    // (or to a different video) never leaves a stale lambda able to fire and
    // corrupt state that belongs to the new selection.
    if (frameSink_) {
        frameSink_->disconnect();
        frameSink_->deleteLater();
        frameSink_ = nullptr;
    }
    pending_video_path_.clear();

    if (isVideoFile(absoluteFilePath)) {
        mediaPlayer->stop();

        drop_preview_rect_ = nullptr; // scene->clear() below will delete it
        scene->clear();
        videoItem = nullptr;
        tag_rect_items_.clear();
        tag_rect_descriptors_.clear();
        image_size_ = QSizeF();

        pending_video_path_ = absoluteFilePath;
        is_video_ = true;
        view->setAcceptDrops(false);
        view->setDragMode(QGraphicsView::NoDrag);
        controlBar_->show();
        playPauseButton_->setText("Play");

        // Capture the first decoded frame and display it as a static thumbnail.
        // A temporary QVideoSink receives frames without touching the scene's
        // videoItem, which is only created when the user actually clicks Play.
        frameSink_ = new QVideoSink(this);

        // Capture the exact sink pointer so the lambda can detect whether it
        // has been superseded (frameSink_ replaced or nulled) before it fires.
        QVideoSink* thisSink = frameSink_;
        connect(frameSink_, &QVideoSink::videoFrameChanged, this,
                [this, thisSink](const QVideoFrame &frame) {
            // Bail out if this callback is stale (a newer selection already
            // replaced frameSink_) or the frame contains no data.
            if (!frame.isValid() || thisSink != frameSink_) return;

            frameSink_->disconnect();
            frameSink_->deleteLater();
            frameSink_ = nullptr;
            mediaPlayer->stop();

            QImage img = frame.toImage();
            const QSize viewSize = view->size();
            const int targetW = viewSize.width()  * 2;
            const int targetH = viewSize.height() * 2;
            if (!img.isNull() && (img.width() > targetW || img.height() > targetH))
                img = img.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // preview(QImage) clears pending_video_path_, but we still need it
            // so the Play button knows to set up the videoItem on first press.
            QString savedPath = pending_video_path_;
            preview(img);       // displays the frame; also sets is_video_=false and hides controlBar_
            pending_video_path_ = savedPath;
            is_video_ = true;   // restore video state so freshen() and other callers behave correctly
            controlBar_->show();
            playPauseButton_->setText("Play");
        });

        mediaPlayer->setVideoSink(frameSink_);
        mediaPlayer->setSource(QUrl::fromLocalFile(absoluteFilePath));
        mediaPlayer->play();

        view->show();
        return;
    }

    QImage image;
    if (QFileInfo(absoluteFilePath).suffix().toLower() == "heic") {
        image = ExifParser::loadHeifImage(absoluteFilePath);
    } else {
        QImageReader ir(absoluteFilePath);
        ir.setAutoTransform(true);
        image = ir.read();
        if (image.isNull())
            QMessageBox::critical(nullptr, "Error", "Failed to load image: " + ir.errorString());
    }

    // Pre-scale to 2× the view size using Qt's software area-averaging scaler.
    // QGraphicsView's bilinear transform alone produces aliasing/moiré when
    // downscaling large images (e.g. 4000px → 600px) on Linux.  Doing the
    // heavy reduction here in software — where SmoothTransformation uses a
    // proper low-pass filter — leaves only a small residual scale for the
    // view transform to handle cleanly.
    const QSize viewSize = view->size();
    const int targetW = viewSize.width()  * 2;
    const int targetH = viewSize.height() * 2;
    if (!image.isNull() && (image.width() > targetW || image.height() > targetH))
        image = image.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
    if (volumePopup_) volumePopup_->hide();
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
        connect(item, &TagRectItem::deleteRequested, this,
                [this, normRectPtr](const QRectF &) {
            emit tagRectDeleteRequested(*normRectPtr);
        });
        connect(item, &TagRectItem::findPersonRequested, this,
                [this, normRectPtr](const QRectF &) {
            emit tagRectFindPersonRequested(*normRectPtr);
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

// ---------------------------------------------------------------------------
// Nav button overlay
// ---------------------------------------------------------------------------

/*! \brief Repositions the left and right overlay buttons over the view. */
void PreviewContainer::repositionNavButtons()
{
    if (!navLeftButton_ || !navRightButton_) return;
    const QRect vr = view->geometry();
    if (vr.isEmpty()) return;
    const int bw = navLeftButton_->width();
    const int bh = navLeftButton_->height();
    static constexpr int kMargin = 16;
    const int y = vr.top() + (vr.height() - bh) / 2;
    navLeftButton_->move(vr.left() + kMargin, y);
    navRightButton_->move(vr.right() - kMargin - bw, y);
    navLeftButton_->raise();
    navRightButton_->raise();
}

/*! \brief Fades the nav overlay buttons to fully visible or hidden.
 *
 * \param visible True to fade in, false to fade out.
 */
void PreviewContainer::setNavButtonsVisible(bool visible)
{
    if (navButtonsVisible_ == visible) return;
    navButtonsVisible_ = visible;

    const qreal target  = visible ? 1.0 : 0.0;
    const qreal current = navLeftOpacity_ ? navLeftOpacity_->opacity() : 0.0;
    if (navFadeAnim_->state() == QAbstractAnimation::Running)
        navFadeAnim_->stop();
    navFadeAnim_->setStartValue(current);
    navFadeAnim_->setEndValue(target);
    navFadeAnim_->start();
}

/*! \brief Repositions nav buttons when the widget is resized.
 *
 * \param event The resize event.
 */
void PreviewContainer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    repositionNavButtons();
}

/*! \brief Tracks mouse position on the view to drive nav button fade in/out.
 *
 * On MouseMove over the view: fades buttons in when cursor is within
 * kNavHotZoneWidth pixels of either horizontal edge, out otherwise.
 * On Leave from the view or a button: fades out unless cursor moved
 * directly onto the other interactive element.
 *
 * \param obj The watched object.
 * \param e   The event.
 * \return False — events are never consumed.
 */
bool PreviewContainer::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == view) {
        if (e->type() == QEvent::KeyPress) {
            // QGraphicsView consumes Left/Right for scrolling; intercept them here
            // so they reach the navigation logic instead.
            auto *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Left) {
                emit navigateRequested(-1);
                return true;
            }
            if (ke->key() == Qt::Key_Right) {
                emit navigateRequested(+1);
                return true;
            }
        } else if (e->type() == QEvent::MouseButtonPress && is_video_) {
            if (static_cast<QMouseEvent*>(e)->button() == Qt::LeftButton) {
                playPauseButton_->click();
                return true;
            }
        } else if (e->type() == QEvent::MouseMove) {
            const int x = static_cast<QMouseEvent*>(e)->pos().x();
            const bool inZone = x < kNavHotZoneWidth ||
                                x > view->width() - kNavHotZoneWidth;
            setNavButtonsVisible(inZone);
        } else if (e->type() == QEvent::Leave) {
            const QPoint gc = QCursor::pos();
            const bool onLeft  = navLeftButton_  &&
                                 navLeftButton_->rect().contains(
                                     navLeftButton_->mapFromGlobal(gc));
            const bool onRight = navRightButton_ &&
                                 navRightButton_->rect().contains(
                                     navRightButton_->mapFromGlobal(gc));
            if (!onLeft && !onRight)
                setNavButtonsVisible(false);
        }
    } else if (obj == navLeftButton_ || obj == navRightButton_) {
        if (e->type() == QEvent::Enter) {
            // Mouse entered a button directly (bypassing the view's hot-zone check
            // because the button sits on top of the view and captures the mouse).
            setNavButtonsVisible(true);
        } else if (e->type() == QEvent::Leave) {
            const QPoint posInView = view->mapFromGlobal(QCursor::pos());
            const int x = posInView.x();
            const bool inZone = x >= 0 && x < view->width() &&
                                (x < kNavHotZoneWidth ||
                                 x > view->width() - kNavHotZoneWidth);
            if (!inZone)
                setNavButtonsVisible(false);
        }
    }
    return QWidget::eventFilter(obj, e);
}

// Required so that moc processes TagRectItem (Q_OBJECT in a .cpp file).
#include "previewcontainer.moc"
