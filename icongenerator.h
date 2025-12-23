#ifndef ICONGENERATOR_H
#define ICONGENERATOR_H

#include <QObject>
#include <QPixmap>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QPainter>

class IconGenerator : public QObject
{
    Q_OBJECT

public:
    IconGenerator(QObject *parent);
    static QPixmap generateIcon(const QString absoluteFileName);

private:
    static QPixmap makeSquareIcon(const QPixmap &source, int size);

};

#endif // ICONGENERATOR_H
