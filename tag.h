#ifndef TAG_H
#define TAG_H

#include <QObject>
#include <QDataStream>
#include "tagfamily.h"

/*! \brief Represents a single tag that can be applied to media files.
 *
 * Each Tag has a name, a pointer to its owning TagFamily, and a dirty flag
 * that is set when the name is changed so that unsaved changes can be detected.
 */
class Tag : public QObject
{
    Q_OBJECT

private:
    QString tag_name_;           ///< The display name of this tag.
    bool dirty_flag_ = false;    ///< True when the tag name has been changed since the last save.

public:
    /*! \brief Constructs a default Tag with an empty name and a new default TagFamily.
     *
     * \param parent Optional Qt parent object.
     */
    explicit Tag(QObject *parent = nullptr);

    /*! \brief Pointer to the TagFamily that owns this tag. */
    TagFamily* tagFamily;

    /*! \brief Constructs a Tag belonging to the given family with the given name.
     *
     * \param tf     The TagFamily this tag belongs to.
     * \param t      The tag name string.
     * \param parent Optional Qt parent object.
     */
    Tag(TagFamily* tf, QString t, QObject *parent = nullptr);

    /*! \brief Sets the tag name and marks the dirty flag if the name changed.
     *
     * \param tagName The new name for this tag.
     */
    void setName(QString tagName);

    /*! \brief Returns the current tag name.
     *
     * \return The tag name string.
     */
    QString getName() const;

    /*! \brief Returns whether the tag name has been modified since the last save.
     *
     * \return True if the dirty flag is set.
     */
    bool dirtyFlag() const;

    /*! \brief Sets the dirty flag to force the tag to be treated as modified. */
    void markDirty();

    /*! \brief Clears the dirty flag after changes have been persisted. */
    void clearDirtyFlag();

    /*! \brief Sets the owning tag family and marks the dirty flag.
     *
     * \param family The new TagFamily this tag should belong to.
     */
    void setTagFamily(TagFamily* family);

    /*! \brief Equality operator comparing tag name and family name.
     *
     * \param other The other Tag to compare against.
     * \return True if both the tag name and family name match.
     */
    bool operator==(const Tag& other) const;

signals:
    /*! \brief Emitted when the tag name is changed via setName(). */
    void nameChanged();

};

/*! \brief Serialises a Tag's name to a QDataStream for drag-and-drop transfer.
 *
 * \param out The output data stream.
 * \param t   The Tag to serialise.
 * \return The output stream after writing.
 */
QDataStream &operator<<(QDataStream &out, const Tag &t);

/*! \brief Deserialises a Tag's name from a QDataStream during drag-and-drop.
 *
 * \param in The input data stream.
 * \param t  The Tag to populate with the read name.
 * \return The input stream after reading.
 */
QDataStream &operator>>(QDataStream &in, Tag &t);

#endif // TAG_H
