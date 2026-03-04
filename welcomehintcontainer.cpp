#include "welcomehintcontainer.h"
#include "flowlayout.h"

#include <QDataStream>
#include <QDropEvent>

/*! \brief Constructs a WelcomeHintContainer with a FlowLayout installed.
 *
 * \param parent Optional Qt parent widget.
 */
WelcomeHintContainer::WelcomeHintContainer(QWidget *parent)
    : TagContainer(parent)
{
    setLayout(new FlowLayout(this));
}

/*! \brief Creates and configures the welcome_label_ with \p hint text. */
void WelcomeHintContainer::initLabel(const QString &hint)
{
    welcome_label_ = new QLabel(hint);
    welcome_label_->setTextFormat(Qt::RichText);
    welcome_label_->setAlignment(Qt::AlignCenter);
    welcome_label_->setWordWrap(true);
    welcome_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    welcome_label_->setEnabled(false);
}

/*! \brief Sets up the welcome hint inside a splitter parent.
 *
 * \param sa       The scroll area to reveal on dismissal.
 * \param sp       The splitter that owns the scroll area.
 * \param idx      Insertion index for the hint label.
 * \param hint     The hint text to display.
 */
void WelcomeHintContainer::setupWelcome(QScrollArea *sa, QSplitter *sp, int idx, const QString &hint)
{
    scroll_area_     = sa;
    parent_splitter_ = sp;
    initLabel(hint);
    sp->insertWidget(idx, welcome_label_);
    sa->hide();
}

/*! \brief Sets up the welcome hint inside a layout parent.
 *
 * \param sa   The scroll area to reveal on dismissal.
 * \param lo   The layout that owns the scroll area.
 * \param idx  Insertion index for the hint label.
 * \param hint The hint text to display.
 */
void WelcomeHintContainer::setupWelcome(QScrollArea *sa, QVBoxLayout *lo, int idx, const QString &hint)
{
    scroll_area_   = sa;
    parent_layout_ = lo;
    initLabel(hint);
    lo->insertWidget(idx, welcome_label_);
    sa->hide();
}

/*! \brief Un-grays the hint label and makes it interactive.
 *
 * No-op if the label is already active or has been dismissed.
 */
void WelcomeHintContainer::activateWelcome()
{
    if (!welcome_label_ || welcome_label_->isEnabled())
        return;
    welcome_label_->setEnabled(true);
    if (accepts_click_) welcome_label_->setCursor(Qt::PointingHandCursor);
    if (accepts_drop_)  welcome_label_->setAcceptDrops(true);
    welcome_label_->installEventFilter(this);
}

/*! \brief Deletes the hint label and reveals the scroll area.
 *
 * Captures splitter sizes before deletion so the revealed scroll area inherits
 * the label's space.  Emits welcomeDismissed() on success.  No-op if already dismissed.
 */
void WelcomeHintContainer::dismissWelcome()
{
    if (!welcome_label_) return;
    if (parent_splitter_) {
        QList<int> sz = parent_splitter_->sizes();
        delete welcome_label_; welcome_label_ = nullptr;
        scroll_area_->show();
        parent_splitter_->setSizes(sz.mid(0, sz.size() - 1));
    } else {
        delete welcome_label_; welcome_label_ = nullptr;
        scroll_area_->show();
    }
    emit welcomeDismissed();
}

/*! \brief Returns true if the welcome hint label is currently showing. */
bool WelcomeHintContainer::isWelcomeShowing() const
{
    return welcome_label_ != nullptr;
}

/*! \brief Enables click-to-dismiss behaviour on the hint label.
 *
 * \param v True to dismiss on a mouse-button press.
 */
void WelcomeHintContainer::setAcceptsClickToDismiss(bool v)
{
    accepts_click_ = v;
}

/*! \brief Enables drop-to-dismiss behaviour on the hint label.
 *
 * \param v True to dismiss (and emit tagDroppedOnWelcome) on a tag drop.
 */
void WelcomeHintContainer::setAcceptsDropToDismiss(bool v)
{
    accepts_drop_ = v;
}

/*! \brief Intercepts mouse and drag events on the hint label.
 *
 * \param obj   The object that received the event.
 * \param event The event to inspect.
 * \return true if the event was consumed; false to pass it on.
 */
bool WelcomeHintContainer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != welcome_label_) return QWidget::eventFilter(obj, event);

    if (accepts_click_ && event->type() == QEvent::MouseButtonPress) {
        dismissWelcome();
        return true;
    }

    if (accepts_drop_) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto *de = static_cast<QDragMoveEvent *>(event);
            if (de->mimeData()->hasFormat("application/x-dnditemdata"))
                de->acceptProposedAction();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            auto *de = static_cast<QDropEvent *>(event);
            if (de->mimeData()->hasFormat("application/x-dnditemdata")) {
                QByteArray data = de->mimeData()->data("application/x-dnditemdata");
                QDataStream stream(&data, QIODevice::ReadOnly);
                QString family, tagName;
                QPoint offset;
                stream >> family >> tagName >> offset;
                de->acceptProposedAction();
                dismissWelcome();
                emit tagDroppedOnWelcome(family, tagName);
            }
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
