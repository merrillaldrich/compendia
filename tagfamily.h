#ifndef TAGFAMILY_H
#define TAGFAMILY_H

#include <QObject>
#include <QString>
#include <QColor>

/*! \brief Represents a named group of related tags, each with an auto-assigned colour.
 *
 * TagFamily acts as the conceptual container for one or more Tag objects.
 * Colours are assigned automatically from a rotating hue sequence so that
 * different families are visually distinct.
 */
class TagFamily : public QObject
{
    Q_OBJECT

private:
    QString tag_family_name_;           ///< The display name of this tag family.
    QColor tag_family_color_;           ///< Auto-assigned colour used to visually distinguish this family.
    bool dirty_flag_ = false;           ///< True when the family name has been changed since the last save.
    static int next_hue_;               ///< Next hue value in the rotating colour sequence (set in .cpp).
    static int starting_hue_;          ///< Initial hue value, used to reset the sequence.

    /*! \brief Generates the next colour in the rotating hue sequence.
     *
     * \return A new QColor for the next tag family.
     */
    static QColor generateNextColor();

public:
    /*! \brief Constructs a TagFamily with an empty name and an auto-assigned colour.
     *
     * \param parent Optional Qt parent object.
     */
    explicit TagFamily(QObject *parent = nullptr);

    /*! \brief Constructs a TagFamily with the given name and an auto-assigned colour.
     *
     * \param tf     The family name string.
     * \param parent Qt parent object.
     */
    TagFamily(QString tf, QObject *parent);

    /*! \brief Sets the family name and marks the dirty flag if the name changed.
     *
     * \param tagFamilyName The new name for this family.
     */
    void setName(QString tagFamilyName);

    /*! \brief Returns the current family name.
     *
     * \return The family name string.
     */
    QString getName() const;

    /*! \brief Returns the colour assigned to this family.
     *
     * \return The family colour.
     */
    QColor getColor() const;

    /*! \brief Returns whether the family name has been modified since the last save.
     *
     * \return True if the dirty flag is set.
     */
    bool dirtyFlag() const;

    /*! \brief Clears the dirty flag after changes have been persisted. */
    void clearDirtyFlag();

    /*! \brief Resets the colour-generation sequence back to the starting hue. */
    static void restartColorSequence();

protected:

signals:
    /*! \brief Emitted when the family name is changed via setName(). */
    void nameChanged();

};

/*! \brief Serialises a TagFamily's name to a QDataStream for drag-and-drop transfer.
 *
 * \param out The output data stream.
 * \param t   The TagFamily to serialise.
 * \return The output stream after writing.
 */
QDataStream &operator<<(QDataStream &out, const TagFamily &t);

/*! \brief Deserialises a TagFamily's name from a QDataStream during drag-and-drop.
 *
 * \param in The input data stream.
 * \param t  The TagFamily to populate with the read name.
 * \return The input stream after reading.
 */
QDataStream &operator>>(QDataStream &in, TagFamily &t);

#endif // TAGFAMILY_H
