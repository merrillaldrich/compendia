#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPainter>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>
#include <QApplication>
#include <QLayout>
#include "tag.h"
#include "clickablelabel.h"
#include "variablewidthlineedit.h"
#include "tagwidgetclosebutton.h"
#include "mainwindow.h"
#include "taggingwidget.h"


class TagWidget : public TaggingWidget
{
    Q_OBJECT

private:
    QString edit_status_ = "Read";
    VariableWidthLineEdit* line_edit_;
    ClickableLabel* label_;
    QPoint drag_start_position_;
    bool in_library_ = false;
    Tag* tag_;

public:
    explicit TagWidget(QWidget *parent = nullptr);

    TagWidget(Tag *tag, QWidget *parent);

    void startEdit();
    void endEdit();
    Tag* getTag();
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void adjustCustomWidth();

private slots:
    void onLineEditEditingFinished();
    void onLabelClicked(QMouseEvent *event);
    void onTagNameChanged();
    void onTextEdited();
    void onCloseButtonClicked();

signals:
    void deleteRequested(Tag *t);
    void tagNameChanged(Tag *t);
};

#endif // TAGWIDGET_H
