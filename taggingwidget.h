#ifndef TAGGINGWIDGET_H
#define TAGGINGWIDGET_H

#include <QObject>
#include <QWidget>
#include <QColor>

class TaggingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaggingWidget(QWidget *parent = nullptr);
    TaggingWidget(QColor color, QWidget *parent);
    QColor base_color_;

};


#endif // TAGGINGWIDGET_H
