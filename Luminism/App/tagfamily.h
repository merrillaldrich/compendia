#ifndef TAGFAMILY_H
#define TAGFAMILY_H

#include <QObject>

class TagFamily : public QObject
{
    Q_OBJECT
public:
    QString tagFamilyName = "";
    explicit TagFamily(QObject *parent = nullptr);
    TagFamily(QString tf, QObject *parent = nullptr);

signals:
};

#endif // TAGFAMILY_H
