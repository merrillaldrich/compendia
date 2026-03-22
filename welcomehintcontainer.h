#ifndef WELCOMEHINTCONTAINER_H
#define WELCOMEHINTCONTAINER_H

#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <QDragMoveEvent>
#include <QShowEvent>
#include "tagcontainer.h"

/*! \brief Intermediate base class that adds a dismissible welcome hint label to TagContainer.
 *
 * Manages an external hint label placed in the parent splitter or layout alongside
 * the scroll area.  The hint is shown grayed-out until activateWelcome() is called,
 * then becomes interactive (click-to-dismiss and/or drop-to-dismiss depending on
 * which flags are set by the subclass).  On dismissal the scroll area is revealed
 * and welcomeDismissed() is emitted.
 *
 * Subclasses call setAcceptsClickToDismiss() and/or setAcceptsDropToDismiss() in
 * their constructors to opt in to each behaviour.
 */
class WelcomeHintContainer : public TagContainer {
    Q_OBJECT
public:
    /*! \brief Constructs a WelcomeHintContainer with a FlowLayout installed.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit WelcomeHintContainer(QWidget *parent = nullptr);

    /*! \brief Sets up the welcome hint inside a splitter parent.
     *
     * Inserts the hint label at \p idx in \p splitter and hides \p sa.
     * Must be called once from the MainWindow constructor.
     *
     * \param sa       The scroll area to reveal on dismissal.
     * \param splitter The splitter that owns the scroll area.
     * \param idx      Insertion index for the hint label.
     * \param hint     The hint text to display.
     */
    void setupWelcome(QScrollArea *sa, QSplitter *splitter, int idx, const QString &hint);

    /*! \brief Sets up the welcome hint inside a layout parent.
     *
     * Inserts the hint label at \p idx in \p layout and hides \p sa.
     * Must be called once from the MainWindow constructor.
     *
     * \param sa     The scroll area to reveal on dismissal.
     * \param layout The layout that owns the scroll area.
     * \param idx    Insertion index for the hint label.
     * \param hint   The hint text to display.
     */
    void setupWelcome(QScrollArea *sa, QVBoxLayout *layout, int idx, const QString &hint);

    /*! \brief Un-grays the hint label and makes it interactive.
     *
     * No-op if the label is already active or has been dismissed.
     */
    void activateWelcome();

    /*! \brief Deletes the hint label and reveals the scroll area.
     *
     * Emits welcomeDismissed() on success.  No-op if already dismissed.
     */
    void dismissWelcome();

    /*! \brief Returns true if the welcome hint label is currently showing. */
    bool isWelcomeShowing() const;

    /*! \brief Registers a background SVG hint icon to be shown after the welcome is dismissed.
     *
     * The icon is painted centred in the scroll area's viewport, behind the scrollable
     * content, so it stays fixed on screen while tags scroll over it.
     *
     * \param svgPath Qt resource path to the SVG file.
     * \param w       Render width in pixels.
     * \param h       Render height in pixels.
     */
    void setHint(const QString &svgPath, int w, int h);

signals:
    /*! \brief Emitted when the welcome hint is dismissed by the user. */
    void welcomeDismissed();

    /*! \brief Emitted when a tag is dropped onto the welcome hint.
     *
     * \param family  The tag family name of the dropped tag.
     * \param tagName The tag name of the dropped tag.
     */
    void tagDroppedOnWelcome(const QString &family, const QString &tagName);

protected:
    /*! \brief Enables click-to-dismiss behaviour on the hint label.
     *
     * \param v True to dismiss on a mouse-button press.
     */
    void setAcceptsClickToDismiss(bool v);

    /*! \brief Enables drop-to-dismiss behaviour on the hint label.
     *
     * \param v True to dismiss (and emit tagDroppedOnWelcome) on a tag drop.
     */
    void setAcceptsDropToDismiss(bool v);

    /*! \brief Intercepts mouse and drag events on the hint label.
     *
     * \param obj   The object that received the event.
     * \param event The event to inspect.
     * \return true if the event was consumed; false to pass it on.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /*! \brief Creates the hint overlay widget on the viewport when the container first shows. */
    void showEvent(QShowEvent *event) override;

private:
    /*! \brief Creates and configures the welcome_label_ with \p hint text. */
    void initLabel(const QString &hint);

    /*! \brief Creates the hint overlay (if not yet created) and sizes it to the viewport.
     *
     * Safe to call multiple times; idempotent once the overlay exists.
     * Uses scroll_area_->viewport() directly so it works regardless of
     * when parentWidget() is resolved.
     */
    void ensureHintOverlay();

    QLabel      *welcome_label_   = nullptr;
    QScrollArea *scroll_area_     = nullptr;
    QSplitter   *parent_splitter_ = nullptr;
    QVBoxLayout *parent_layout_   = nullptr;
    bool accepts_click_ = false;
    bool accepts_drop_  = false;

    QWidget    *hint_overlay_  = nullptr;
    QString     hint_svg_path_;
    int         hint_w_        = 0;
    int         hint_h_        = 0;
};

#endif // WELCOMEHINTCONTAINER_H
