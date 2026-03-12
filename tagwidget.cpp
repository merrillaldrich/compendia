#include "tagwidget.h"

/*! \brief Constructs a default, empty TagWidget.
 *
 * \param parent Optional Qt parent widget.
 */
TagWidget::TagWidget(QWidget *parent)
    : TaggingWidget{parent}
{}

/*! \brief Constructs a TagWidget bound to the given Tag and styled with its family colour.
 *
 * \param tag    The Tag this widget represents.
 * \param parent Qt parent widget.
 */
TagWidget::TagWidget(Tag *tag, QWidget *parent)
    : TaggingWidget{parent}{
    tag_ = tag;
    base_color_ = tag->tagFamily->getColor();

    // Connect this widget to its tag's name changed event
    connect(tag_, &Tag::nameChanged, this, &TagWidget::onTagNameChanged);

    // Must set a default size here even though the width will be adjusted for the text content later
    setFixedWidth(36);
    setFixedHeight(28);

    QSizePolicy policy = this->sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Minimum);

    setAttribute(Qt::WA_TranslucentBackground);

    TagWidgetCloseButton* closeButton = new TagWidgetCloseButton(this);
    closeButton->setMinimumSize(20,20);
    connect(closeButton, &TagWidgetCloseButton::clicked, this, &TagWidget::onCloseButtonClicked);

    line_edit_ = new VariableWidthLineEdit(this);
    line_edit_->move(20, 0);

    //line_edit_->setFixedWidth(36);
    line_edit_->setFixedHeight(line_edit_->height());

    line_edit_->hide();

    // Connect this widget to the line edit, to keep this wide enough
    // on screen for longer values to be entered
    connect(line_edit_, &QLineEdit::textEdited, this, &TagWidget::onTextEdited);

    label_ = new ClickableLabel(this);
    label_->move(26, 0);
    //label_->setFixedWidth(32);
    label_->setFixedHeight(label_->height());
    label_->setText(tag->getName());

    adjustCustomWidth();

    label_->show();

    connect(line_edit_, &QLineEdit::editingFinished, this, &TagWidget::onLineEditEditingFinished);
    connect(line_edit_, &VariableWidthLineEdit::escapePressed, this, &TagWidget::endEdit);
    connect(label_, &ClickableLabel::clicked, this, &TagWidget::onLabelClicked);
}

/*! \brief Slot called when the line edit finishes editing; commits the change via endEdit(). */
void TagWidget::onLineEditEditingFinished(){
    if (edit_status_ == "Edit")
        endEdit();
}

/*! \brief Slot called when the label is clicked; enters inline-edit mode via startEdit().
 *
 * \param event The mouse event from the click.
 */
void TagWidget::onLabelClicked(QMouseEvent *event){
    qDebug() << "Tag label clicked";
    startEdit();
    event->accept();
}

/*! \brief Slot called as the user types in the line edit; adjusts the widget width. */
void TagWidget::onTextEdited(){
    adjustCustomWidth();
}

/*! \brief Slot called when the close button is clicked; emits deleteRequested(). */
void TagWidget::onCloseButtonClicked(){
    emit(deleteRequested(this->tag_));
}

/*! \brief Overrides the Qt base-class paint handler to draw the coloured rounded-rectangle background.
 *
 * \param event The paint event.
 */
void TagWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QPen p = QPen(Qt::NoPen); //QBrush(QColor("black")), 2);
    //painter.setPen(p);

    QBrush b = QBrush(base_color_);
    painter.setPen(p);
    painter.setBrush(b);
    painter.setRenderHint(QPainter::Antialiasing);

    int inset = 2;
    int leftX = inset;
    int topY = inset;
    int rightX = width() - (inset * 2);
    int bottomY = height() - (inset * 2);
    int cornerRadius = 12;

    painter.drawRoundedRect(QRect(leftX, topY, rightX, bottomY), cornerRadius, cornerRadius);
}

/*! \brief Switches the widget into inline-edit mode, showing the line edit. */
void TagWidget::startEdit(){

    edit_status_ = "Edit";
    label_->hide();
    line_edit_->setText(tag_->getName());
    line_edit_->updateWidth();   // size the line edit for the pre-loaded text before adjustCustomWidth reads it
    adjustCustomWidth();
    line_edit_->show();
    line_edit_->setFocus();
}

/*! \brief Commits the edited name to the Tag and switches back to read mode.
 *
 * If the new name matches an existing tag in the same family, the two tags are
 * merged (this tag is folded into the pre-existing one) rather than creating a
 * duplicate.
 */
void TagWidget::endEdit(){

    edit_status_ = "Read";

    MainWindow* mainWin = qobject_cast<MainWindow*>(this->window());
    const QString newName = line_edit_->text();

    // Abandon a newly created tag if no non-whitespace name was entered
    if (!in_library_ && newName.trimmed().isEmpty()) {
        emit abandonRequested(this);
        return;
    }

    // --- Collision / merge check ---
    Tag* collision = mainWin
        ? mainWin->core->getTag(tag_->tagFamily->getName(), newName)
        : nullptr;
    if (collision && collision != tag_) {
        // Disconnect before merge so a nameChanged on the dying object
        // does not fire into this already-departing widget.
        disconnect(tag_, &Tag::nameChanged, this, &TagWidget::onTagNameChanged);
        mainWin->core->mergeTag(tag_, collision);
        // tag_ is now scheduled for deletion; this widget is removed by the
        // library refresh triggered inside mergeTag → tagLibraryChanged.
        return;
    }

    // --- Normal rename ---
    tag_->setName(newName);

    // If this tag isn't represented in the library then add it.
    // Should happen once on creation; After that edits already apply
    // in the library because this is holding a pointer

    if(! in_library_) {
        mainWin->core->addLibraryTag(tag_);
        in_library_ = true;
    }

    line_edit_->clearFocus();
    line_edit_->hide();
    label_->setText(tag_->getName());
    adjustCustomWidth();
    label_->show();
}

/*! \brief Overrides the Qt base-class mouse-press handler to record the drag start position.
 *
 * \param event The mouse press event.
 */
void TagWidget::mousePressEvent(QMouseEvent *event){

    // To determine if this should be a drag operation or just a click, record the location of the mouse when left button pressed
    // then use the value in MouseMoveEvent() to check the distance the mouse was moved

    if (event->button() == Qt::LeftButton){
        drag_start_position_ = event->pos();
    }

}

/*! \brief Overrides the Qt base-class mouse-move handler to initiate a drag operation.
 *
 * \param event The mouse move event.
 */
void TagWidget::mouseMoveEvent(QMouseEvent *event){

    // Start a drag, but only if the mouse was moved with the left button pressed

    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - drag_start_position_).manhattanLength() < QApplication::startDragDistance())
        return;

    TagWidget* draggedTag = this;
    Tag* t = draggedTag->tag_;
    QString tagName = t->getName();
    QString tagFamilyName = t->tagFamily->getName();

    // Render the dragged widget into an image to show what is draggin'
    QPixmap dragimage = this->grab();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << tagFamilyName << tagName << QPoint(event->position().toPoint() - draggedTag->pos());

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);

    // Bundle the image and mime data into a QDrag object
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(dragimage);
    drag->setHotSpot(event->position().toPoint());
    drag->exec();
}

/*! \brief Slot called when the underlying Tag's name changes; updates the label text. */
void TagWidget::onTagNameChanged(){
    label_->setText(tag_->getName());
    label_->adjustSize(); // Note this is only changing the width, height is fixed with a policy
    adjustCustomWidth();
    emit tagNameChanged(tag_);
}

/*! \brief Returns the Tag pointer this widget represents.
 *
 * \return The associated Tag.
 */
Tag* TagWidget::getTag(){
    return tag_;
}

/*! \brief Returns the preferred size of the widget.
 *
 * \return The size hint.
 */
QSize TagWidget::sizeHint() const {
    return minimumSize();
}

/*! \brief Returns the minimum size of the widget.
 *
 * \return The minimum size hint.
 */
QSize TagWidget::minimumSizeHint() const {
    return minimumSize();
}

/*! \brief Recalculates and applies the widget width to fit the current label or line-edit text. */
void TagWidget::adjustCustomWidth(){
    // Set the width of this widget to a custom size based on the tag name, but only if the tag name
    // has a value. Otherwise we need the default width so it doesn't collapse
    // Use the label width when not editing the value, but the line edit widget width in edit mode

    // This should theoretically be more reliable than label_.adjustSize()
    QFontMetrics fm = QFontMetrics(font());
    if(label_->text().length() < 4){
        label_->setFixedWidth(fm.horizontalAdvance("WWW") + 4);
    } else {
        label_->setFixedWidth(fm.horizontalAdvance(label_->text()) + 4);
    }

    if(edit_status_ == "Edit"){
        setFixedWidth(line_edit_->width() + 36);
        // Notify the parent TagFamilyWidget directly so it can resize without
        // posting a LayoutRequest event (which causes re-entrant processing in
        // the GTK file dialog's event loop).
        emit widthChangedDuringEdit();
    } else {
        if(label_->text().length() > 0) {
            setFixedWidth(label_->width() + 36);
        } else {
            label_->setFixedWidth(24);
            setFixedWidth(36);
        }
        updateGeometry();
    }
    update();
}
