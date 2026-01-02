#ifndef TAGGINGWIDGET_H
#define TAGGINGWIDGET_H

#include <QObject>
#include <QWidget>
#include <QColor>

class TaggingWidget : public QWidget
{
    Q_OBJECT
private:

public:
    explicit TaggingWidget(QWidget *parent = nullptr);
    TaggingWidget(QColor color, QWidget *parent);

protected:
    QColor base_color_;
signals:
};


#endif // TAGGINGWIDGET_H
