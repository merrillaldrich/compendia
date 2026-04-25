#include "tagassignmentcontainer.h"

/*! \brief Constructs an empty TagAssignmentContainer.
 *
 * \param parent Optional Qt parent widget.
 */
TagAssignmentContainer::TagAssignmentContainer(QWidget *parent)
    : WelcomeHintContainer{parent}
{
    setAcceptDrops(true);
    setAcceptsDropToDismiss(true);
    setHint(QStringLiteral(":/resources/tag-hint.svg"), 70, 70);
    setAutoFillBackground(false);
    connect(this, &WelcomeHintContainer::tagDroppedOnWelcome,
            this, [this](const QString &family, const QString &tagName) {
        auto *mw = qobject_cast<MainWindow *>(window());
        if (!mw) return;
        if (Tag *tag = mw->core->getTag(family, tagName)) {
            mw->core->applyTag(tag);
            mw->refreshTagAssignmentArea();
        }
    });
}

/*! \brief Overrides the Qt base-class handler to accept tag drag-enter events.
 *
 * \param event The drag-enter event to evaluate.
 */
void TagAssignmentContainer::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

/*! \brief Overrides the Qt base-class handler to keep accepting tag drag-move events.
 *
 * \param event The drag-move event to evaluate.
 */
void TagAssignmentContainer::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

/*! \brief Overrides the Qt base-class handler to apply the dropped tag to all filtered files.
 *
 * \param event The drop event containing the dragged tag data.
 */
void TagAssignmentContainer::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {

        // Convert the dropped data
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString tagFamilyName;
        QString tagName;
        QPoint offset;

        dataStream >> tagFamilyName >> tagName >> offset;

        // Locate the dropped tag in the tag library
        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());
        if (!mainWin) return;

        Tag* t = mainWin->core->getTag(tagFamilyName, tagName);
        if (!t) return;
        TagFamily* f = t->tagFamily;

        // Identify an existing TagFamilyWidget or make a new TagFamilyWidget if it's missing

        TagWidget* tw = nullptr;
        TagFamilyWidget* tfw = nullptr;

        QList<TagFamilyWidget*> existingTFWidgets = findChildren<TagFamilyWidget*>();

        for(int tfwi = 0; tfwi < existingTFWidgets.count(); ++tfwi){
            TagFamilyWidget* existingTFWidget = existingTFWidgets.at(tfwi);

            if (existingTFWidget->getTagFamily() == f) {
                // Tag family widget exists, use it
                tfw = existingTFWidget;
            }
        }

        if (tfw==nullptr){
            tfw = new TagFamilyWidget(f, this);
            layout()->addWidget(tfw);
            tfw->show();
        }

        // If there are no tag widgets, or there's no tagwidget for the current tag in the current family widget, add a new tagwidget
        QList<TagWidget*> existingTWidgets = tfw->findChildren<TagWidget*>();

        for(int twi = 0; twi < existingTWidgets.count(); ++twi){
            TagWidget* existingTWidget = existingTWidgets.at(twi);
            if(existingTWidget->getTag() == t){
                // Tag widget exists, use it
                tw = existingTWidget;
            }
        }

        if (tw==nullptr){
            tw = new TagWidget(t, tfw);

            if (! connect(tw, &TagWidget::deleteRequested, this, &TagAssignmentContainer::onTagDeleteRequested))
                qWarning() << "Failed to connect tag widget delete to assignment container delete";

            tfw->layout()->addWidget(tw);
            tw->show();
            tfw->refreshMinimumHeight();
        }

        tfw->sort();
        sort();

        // Apply the dropped tag to all visible files
        mainWin->core->applyTag(t);

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }

    } else {
        event->ignore();
    }
}
