#include "tagfamilywidget.h"

TagFamilyWidget::TagFamilyWidget(QWidget *parent)
    : QWidget{parent}
{}

TagFamilyWidget::TagFamilyWidget(TagFamily *tagFamily, QWidget *parent)
    : QWidget{parent}
{
    tag_family_ = tagFamily;

    // Connect this widget to its tag family's name changed event
    connect(tag_family_, &TagFamily::nameChanged, this, &TagFamilyWidget::onTagFamilyNameChanged);

    setMinimumSize(304,64);
    setAttribute(Qt::WA_TranslucentBackground);

    FlowLayout* fl = new FlowLayout(this);
    fl->setContentsMargins(4, 28, 4, 4); //left, top, right, bottom
    fl->setSpacing(2);
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

    connect(line_edit_, &QLineEdit::returnPressed, this, &TagFamilyWidget::onReturnPressed);
    connect(label_, &ClickableLabel::clicked, this, &TagFamilyWidget::onLabelClicked);
}

void TagFamilyWidget::mouseReleaseEvent(QMouseEvent *event){

    Tag* t = new Tag(tag_family_, "", this);
    TagWidget* tw = new TagWidget(t, this);
    layout()->addWidget(tw);
    tw->show();

    // Here we need to force one extra layout computation to get this widget
    // to resized to accomodate the new child widget. Without this
    // the vertical expansion of this tagfamilywidget lags one tag
    // behind:
    layout()->invalidate();
    layout()->activate();
    updateGeometry();

    // Explicitly set the height of this widget to fit all the current
    // children
    this->setMinimumHeight(childrenRect().height() + 4);

    // Put the new tag in edit mode immediately so the user doesn't have to
    // click on it to enter the name
    tw->startEdit();
    event->accept();

    //QWidget::mouseReleaseEvent(event); // Do not call parent b/c we handled the event fully
}

void TagFamilyWidget::onReturnPressed(){
    endEdit();
}

void TagFamilyWidget::onLabelClicked(QMouseEvent *event){
    startEdit();
    event->accept();
}

void TagFamilyWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen p = QPen(QColor("orange")); //QBrush(QColor("black")), 2);
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

    painter.setBrush(QColor("orange"));
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

void TagFamilyWidget::startEdit(){
    edit_status_ = "Edit";
    label_->hide();
    line_edit_->setText(tag_family_->getName());
    line_edit_->show();
    line_edit_->setFocus();
}

void TagFamilyWidget::endEdit(){
    edit_status_ = "Read";
    tag_family_->setName(line_edit_->text());

    // If this tagfamily isn't represented in the library then add it.
    // Should happen once on creation; After that edits already apply
    // in the library because this is holding a pointer

    if(! in_library_) {
        MainWindow *mainWin = qobject_cast<MainWindow*>(this->window());
        mainWin->core->addLibraryTagFamily(tag_family_);
        in_library_ = true;
    }

    line_edit_->clearFocus();
    line_edit_->hide();
    label_->show();
}

void TagFamilyWidget::onTagFamilyNameChanged(){
    label_->setText(tag_family_->getName());
    label_->adjustSize(); // Note this is only changing the width, height is fixed with a policy
    update(0, 0, width(), 26); // Paint a band across the widget for the case where the label became shorter after edit
}

TagFamily* TagFamilyWidget::getTagFamily(){
    return tag_family_;
}

QSize TagFamilyWidget::sizeHint() const {
    return minimumSize();
}
