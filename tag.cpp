#include "tag.h"

/*! \brief Constructs a default Tag with an empty name and a new default TagFamily.
 *
 * \param parent Optional Qt parent object.
 */
Tag::Tag(QObject *parent)
    : QObject{parent}{
    tagFamily = new TagFamily();
    tag_name_ = "";
}

/*! \brief Constructs a Tag belonging to the given family with the given name.
 *
 * \param tf     The TagFamily this tag belongs to.
 * \param t      The tag name string.
 * \param parent Optional Qt parent object.
 */
Tag::Tag(TagFamily* tf, QString t, QObject *parent)
    : QObject{parent}{
    tagFamily = tf;
    tag_name_ = t;
}

/*! \brief Sets the tag name and marks the dirty flag if the name changed.
 *
 * \param tagName The new name for this tag.
 */
void Tag::setName(QString tagName){
    if(tag_name_ != tagName){
        tag_name_ = tagName;
        dirty_flag_ = true;
        emit nameChanged();
    }
}

/*! \brief Returns whether the tag name has been modified since the last save.
 *
 * \return True if the dirty flag is set.
 */
bool Tag::dirtyFlag() const
{
    return dirty_flag_;
}

/*! \brief Clears the dirty flag after changes have been persisted. */
void Tag::clearDirtyFlag()
{
    dirty_flag_ = false;
}

/*! \brief Returns the current tag name.
 *
 * \return The tag name string.
 */
QString Tag::getName() const {
    return tag_name_;
}

/*! \brief Equality operator comparing tag name and family name.
 *
 * \param other The other Tag to compare against.
 * \return True if both the tag name and family name match.
 */
bool Tag::operator==(const Tag& other) const {
    return ( tag_name_ == other.getName() && tagFamily->getName() == other.tagFamily->getName() );
}

/*! \brief Serialises a Tag's name to a QDataStream for drag-and-drop transfer.
 *
 * \param out The output data stream.
 * \param t   The Tag to serialise.
 * \return The output stream after writing.
 */
QDataStream &operator<<(QDataStream &out, const Tag &t) {
    QString name = t.getName();
    out << name;
    return out;
}

/*! \brief Deserialises a Tag's name from a QDataStream during drag-and-drop.
 *
 * \param in The input data stream.
 * \param t  The Tag to populate with the read name.
 * \return The input stream after reading.
 */
QDataStream &operator>>(QDataStream &in, Tag &t) {
    QString newName;
    in >> newName;
    t.setName(newName);
    return in;
}
