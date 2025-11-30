#include "tag.h"

Tag::Tag(QObject *parent)
    : QObject{parent}{
    tagFamily = new TagFamily("");
    tagName = "";

}

Tag::Tag(TagFamily *tf, QString t, QObject *parent)
    : QObject{parent}{
    tagFamily = tf;
    tagName = t;
}
