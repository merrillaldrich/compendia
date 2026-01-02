#include "tagfamily.h"

// Single static variable to hold the hue (color wheel angle) of the last
// used dynamic tag color
int TagFamily::next_hue_ = 20;

TagFamily::TagFamily(QObject *parent)
    : QObject{parent}{
    tag_family_name_ = "";
    tag_family_color_ = TagFamily::generateNextColor();
}

TagFamily::TagFamily(QString tf, QObject *parent = nullptr)
    : QObject{parent}{
    tag_family_name_ = tf;
    tag_family_color_ = TagFamily::generateNextColor();
}

void TagFamily::setName(QString tagFamilyName){
    if(tag_family_name_ != tagFamilyName){
        tag_family_name_ = tagFamilyName;
        emit nameChanged();
    }
}

QString TagFamily::getName() const {
    return tag_family_name_;
}

QColor TagFamily::getColor() const {
    return tag_family_color_;
}

// Overloaded operator<< for writing to QDataStream for drag and drop
QDataStream &operator<<(QDataStream &out, const TagFamily &t) {
    QString name = t.getName();
    out << name;
    return out;
}

// Overload operator>> for reading from QDataStream for drag and drop
QDataStream &operator>>(QDataStream &in, TagFamily &t) { 
    QString newName;
    in >> newName;
    t.setName(newName);
    return in;
}

QColor TagFamily::generateNextColor(){

    int current_hue = next_hue_ % 360;
    double times_around_the_wheel = next_hue_ / 360;

    int current_saturation = 230 - (230 * times_around_the_wheel * 0.2);
    int current_value = 240 - (240 * times_around_the_wheel * 0.07);

    next_hue_ += 32;

    // move faster across greens since people can't perceive the differences
    // as well as other colors

    if (next_hue_ % 360 > 95 && next_hue_ % 360 < 175)
        next_hue_ += 7;

    QColor color = QColor();
    color.setHsv(current_hue, current_saturation, current_value);
    return color;
}
