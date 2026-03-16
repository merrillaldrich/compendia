#include "tagfamilywidget.h"
#include "tagcontainer.h"

/*! \brief Constructs a default, empty TagFamilyWidget.
 *
 * \param parent Optional Qt parent widget.
 */
TagFamilyWidget::TagFamilyWidget(QWidget *parent)
    : TaggingWidget{parent}
{}

/*! \brief Constructs a TagFamilyWidget bound to the given TagFamily.
 *
 * \param tagFamily The TagFamily this widget represents.
 * \param parent    Qt parent widget.
 */
TagFamilyWidget::TagFamilyWidget(TagFamily *tagFamily, QWidget *parent)
    : TaggingWidget{parent}
{
    tag_family_ = tagFamily;
    base_color_ = tagFamily->getColor();

    // Connect this widget to its tag family's name changed event
    connect(tag_family_, &TagFamily::nameChanged, this, &TagFamilyWidget::onTagFamilyNameChanged);

    setMinimumSize(304,64);
    setAttribute(Qt::WA_TranslucentBackground);

    FlowLayout* fl = new FlowLayout(this, -1, 0, 0);
    fl->setContentsMargins(4, 28, 4, 4); //left, top, right, bottom
    this->setLayout(fl);

    line_edit_ = new VariableWidthLineEdit(this);
    line_edit_->move(12, 0);
    line_edit_->setFixedHeight(line_edit_->height());
    line_edit_->hide();

    label_ = new ClickableLabel(this);
    label_->move(18, 0);
    label_->setFixedHeight(label_->height());
    label_->setText(tag_family_->getName());

    // Set the width of this widget's label to a custom size based on the family name, but only
    // if the family name has a value. Otherwise we need the default width so it doesn't collapse.

    if(label_->text().length() > 0){
        label_->adjustSize(); // Note this is only changing the width, height is fixed with a policy
        update(0, 0, width(), 26); // Paint a band across the widget for the case where the label became shorter after edit
    }

    label_->show();

    connect(line_edit_, &QLineEdit::editingFinished, this, &TagFamilyWidget::onLineEditEditingFinished);
    connect(line_edit_, &VariableWidthLineEdit::escapePressed, this, &TagFamilyWidget::endEdit);
    connect(label_, &ClickableLabel::clicked, this, &TagFamilyWidget::onLabelClicked);

    collapseButton_ = new TagFamilyWidgetCollapseButton(this);
    collapseButton_->setFixedSize(20, 20);
    collapseButton_->move(width() - 24, 4);
    connect(collapseButton_, &TagFamilyWidgetCollapseButton::clicked,
            this, &TagFamilyWidget::toggleCollapsed);
}

/*! \brief Overrides the Qt base-class mouse-release handler to add a new tag on click.
 *
 * \param event The mouse release event.
 */
void TagFamilyWidget::mouseReleaseEvent(QMouseEvent *event){

    if (!collapsed_)
        createAndEditNewTag();

    event->accept();
}

/*! \brief Slot called when the line edit finishes editing; commits via endEdit(). */
void TagFamilyWidget::onLineEditEditingFinished(){
    if (edit_status_ == "Edit")
        endEdit();
}

/*! \brief Slot called when the family name label is clicked; enters edit mode via startEdit().
 *
 * \param event The mouse event from the click.
 */
void TagFamilyWidget::onLabelClicked(QMouseEvent *event){
    startEdit();
    event->accept();
}

/*! \brief Overrides the Qt base-class paint handler to draw the family border and label area.
 *
 * \param event The paint event.
 */
void TagFamilyWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen p = QPen(base_color_); //QBrush(QColor("black")), 2);
    painter.setPen(p);

    int inset = 2;
    int leftX = inset;
    int topY = inset;
    int rightX = width() - (inset * 2);
    int bottomY = height() - (inset * 2);
    int cornerRadius = 12;
    int labelAreaHeight = 26;
    int labelAreaWidth = leftX + cornerRadius + label_->width() + 24;

    painter.drawRoundedRect(QRect(leftX, topY, rightX, bottomY), cornerRadius, cornerRadius);

    QPainterPath path;

    painter.setBrush(QColor(base_color_));
    painter.setPen(Qt::NoPen);

    path.moveTo(cornerRadius, topY);
    path.lineTo(labelAreaWidth, topY);
    path.lineTo(labelAreaWidth, labelAreaHeight);
    path.lineTo(leftX, labelAreaHeight);
    path.lineTo(leftX, cornerRadius);
    path.arcTo(leftX, topY, cornerRadius*2, cornerRadius*2, 180, -90);
    path.closeSubpath();

    painter.drawPath(path);
}

/*! \brief Creates a new empty TagWidget, wires its signals, adds it to the layout, and
 *  puts it into edit mode.  Used by both mouseReleaseEvent and endEdit(). */
void TagFamilyWidget::createAndEditNewTag(){
    Tag* t = new Tag(tag_family_, "", this);
    TagWidget* tw = new TagWidget(t, this);
    layout()->addWidget(tw);
    tw->show();

    if (auto *tc = qobject_cast<TagContainer*>(parentWidget()))
        connect(tw, &TagWidget::deleteRequested, tc, &TagContainer::onTagDeleteRequested);

    connect(tw, &TagWidget::widthChangedDuringEdit, this, &TagFamilyWidget::onChildTagWidthChanged);

    connect(tw, &TagWidget::abandonRequested, this, [this](TagWidget *tw) {
        Tag *t = tw->getTag();
        layout()->removeWidget(tw);
        tw->hide();             // Prevent flash: setParent(nullptr) makes a top-level window
        tw->setParent(nullptr);
        tw->deleteLater();
        delete t;
        layout()->invalidate();
        layout()->activate();
        setMinimumHeight(qMax(64, childrenRect().height() + 4));
        updateGeometry();
    });

    tw->startEdit();

    layout()->invalidate();
    layout()->activate();
    updateGeometry();
    setMinimumHeight(childrenRect().height() + 4);
}

/*! \brief Switches the widget into inline-edit mode for the family name. */
void TagFamilyWidget::startEdit(){
    edit_status_ = "Edit";
    label_->hide();
    line_edit_->setText(tag_family_->getName());
    line_edit_->show();
    line_edit_->setFocus();
}

/*! \brief Commits the edited family name and switches back to read mode.
 *
 * If the new name matches an existing tag family, the two families are merged
 * (this family is folded into the pre-existing one) rather than creating a
 * duplicate.
 */
void TagFamilyWidget::endEdit(){
    edit_status_ = "Read";

    MainWindow* mainWin = qobject_cast<MainWindow*>(this->window());
    const QString newName = line_edit_->text();

    // Abandon a newly created family if no non-whitespace name was entered
    if (!in_library_ && newName.trimmed().isEmpty()) {
        disconnect(tag_family_, &TagFamily::nameChanged, this, &TagFamilyWidget::onTagFamilyNameChanged);
        hide();             // Prevent flash: setParent(nullptr) makes a top-level window
        setParent(nullptr);
        tag_family_->deleteLater();
        deleteLater();
        return;
    }

    // --- Collision / merge check ---
    TagFamily* collision = mainWin
        ? mainWin->core->getTagFamily(newName)
        : nullptr;
    if (collision && collision != tag_family_) {
        // Disconnect before merge so a nameChanged on the dying object
        // does not fire into this already-departing widget.
        disconnect(tag_family_, &TagFamily::nameChanged,
                   this, &TagFamilyWidget::onTagFamilyNameChanged);
        mainWin->core->mergeTagFamily(tag_family_, collision);
        // This widget (and tag_family_) are scheduled for deletion by the
        // library refresh triggered inside mergeTagFamily → tagLibraryChanged.
        return;
    }

    // --- Normal rename ---
    tag_family_->setName(newName);

    // If this tagfamily isn't represented in the library then add it.
    // Should happen once on creation; After that edits already apply
    // in the library because this is holding a pointer

    const bool justCreated = !in_library_;
    if (justCreated) {
        mainWin->core->addLibraryTagFamily(tag_family_);
        in_library_ = true;
    }

    line_edit_->clearFocus();
    line_edit_->hide();
    label_->show();

    if (justCreated)
        createAndEditNewTag();
}

/*! \brief Slot called when the underlying TagFamily name changes; refreshes the label. */
void TagFamilyWidget::onTagFamilyNameChanged(){
    label_->setText(tag_family_->getName());
    label_->adjustSize(); // Note this is only changing the width, height is fixed with a policy
    update(0, 0, width(), 26); // Paint a band across the widget for the case where the label became shorter after edit
}

/*! \brief Slot called when a child tag name changes; re-sorts the tag widgets. */
void TagFamilyWidget::onTagNameChanged(){
    sort();
}

/*! \brief Returns the TagFamily pointer this widget represents.
 *
 * \return The associated TagFamily.
 */
TagFamily *TagFamilyWidget::getTagFamily() const{
    return tag_family_;
}

/*! \brief Slot called when the collapse button is clicked; toggles the collapsed state. */
void TagFamilyWidget::toggleCollapsed()
{
    setCollapsed(!collapsed_);
}

/*! \brief Sets the collapsed state directly.
 *
 * \param collapsed true to collapse, false to expand.
 */
void TagFamilyWidget::setCollapsed(bool collapsed)
{
    collapsed_ = collapsed;
    collapseButton_->setCollapsed(collapsed_);

    for (int i = 0; i < layout()->count(); ++i) {
        if (QWidget *w = layout()->itemAt(i)->widget())
            w->setVisible(!collapsed_);
    }

    layout()->invalidate();
    layout()->activate();
    updateGeometry();

    if (collapsed_)
        setMinimumHeight(38);
    else
        setMinimumHeight(qMax(64, childrenRect().height() + 4));

    update();
}

/*! \brief Shows a Collapse All / Expand All context menu when right-clicking the header area. */
void TagFamilyWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // Only respond in the header strip (top ~28 px)
    if (event->pos().y() >= 28) {
        event->ignore();
        return;
    }

    QMenu menu(this);
    menu.addAction("Collapse All", this, [this]() {
        const auto siblings = parentWidget()->findChildren<TagFamilyWidget*>(
            QString(), Qt::FindDirectChildrenOnly);
        for (TagFamilyWidget *w : siblings)
            w->setCollapsed(true);
    });
    menu.addAction("Expand All", this, [this]() {
        const auto siblings = parentWidget()->findChildren<TagFamilyWidget*>(
            QString(), Qt::FindDirectChildrenOnly);
        for (TagFamilyWidget *w : siblings)
            w->setCollapsed(false);
    });
    menu.exec(event->globalPos());
    event->accept();
}

/*! \brief Overrides the Qt base-class resize handler to keep the collapse button anchored.
 *
 * \param event The resize event.
 */
void TagFamilyWidget::resizeEvent(QResizeEvent *event)
{
    TaggingWidget::resizeEvent(event);
    if (collapseButton_)
        collapseButton_->move(width() - 24, 4);
}

/*! \brief Responds to a child TagWidget changing width during editing.
 *
 * Measures the height the FlowLayout needs at the current width and adjusts
 * this widget's minimum height if a row was added or removed.  Called via a
 * direct signal connection, so it runs synchronously in the keystroke call
 * stack rather than being queued — safe to call updateGeometry() here because
 * we are not inside setGeometry or a paint event.
 */
void TagFamilyWidget::onChildTagWidthChanged()
{
    refreshMinimumHeight();
}

/*! \brief Recalculates and applies the correct minimum height for the current set
 *  of child tag widgets.
 *
 * Call this after adding or removing child TagWidget items programmatically so
 * the family widget expands or contracts to fit.
 */
void TagFamilyWidget::refreshMinimumHeight()
{
    if (collapsed_)
        return;

    const int needed = qMax(64, layout()->heightForWidth(contentsRect().width()) + 4);
    if (minimumHeight() == needed)
        return;

    setMinimumHeight(needed);
    updateGeometry();
}

/*! \brief Returns the minimum size as the size hint.
 *
 * \return The size hint.
 */
QSize TagFamilyWidget::sizeHint() const {
    return minimumSize();
}

/*! \brief Sorts the child TagWidget items alphabetically by tag name. */
void TagFamilyWidget::sort() {
    QList<TagWidget*> twlist;
    QLayoutItem* item = nullptr;

    // Temporarily move child widgets to a list
    while ((item = layout()->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            if (auto *tfw = qobject_cast<TagWidget*>(widget)) {
                twlist.append(tfw);
            }
        }
        delete item; // is this needed to avoid QLayoutItem leak?
    }

    // Sort the list (compare pointers)
    std::sort(twlist.begin(), twlist.end(),
              [](TagWidget* a, TagWidget* b) {
                  return a->getTag()->getName() < b->getTag()->getName();
              });

    // Put the widgets back in the layout, in order
    for (TagWidget* w : twlist) {
        layout()->addWidget(w);
    }
}
