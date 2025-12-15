#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPainter>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>
#include <QApplication>
#include "tag.h"
#include "clickablelabel.h"
#include "mainwindow.h"


class TagWidget : public QWidget
{
    Q_OBJECT

private:
    QString edit_status_ = "Read";
    QLineEdit* line_edit_;
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

protected:

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:

    void onReturnPressed();
    void onLabelClicked(QMouseEvent *event);

signals:

};

#endif // TAGWIDGET_H
