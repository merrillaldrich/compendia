#ifndef TAGFAMILY_H
#define TAGFAMILY_H

#include <QObject>
#include <QString>
#include <QColor>

class TagFamily : public QObject
{
    Q_OBJECT

private:
    QString tag_family_name_;
    QColor tag_family_color_;
    static int next_hue_; // Default for static val set in implementation file
    static QColor generateNextColor();

public:
    explicit TagFamily(QObject *parent = nullptr);

    TagFamily(QString tf, QObject *parent);

    void setName(QString tagFamilyName);
    QString getName() const;
    QColor getColor() const;

protected:

signals:
    void nameChanged();

};

// Overload operators >> << for reading and writing QDataStream for drag and drop

QDataStream &operator<<(QDataStream &out, const TagFamily &t);
QDataStream &operator>>(QDataStream &in, TagFamily &t);

#endif // TAGFAMILY_H
