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
    int color_index_ = 0;               ///< The hue-sequence index used to generate this family's colour.
    bool dirty_flag_ = false;           ///< True when the family name has been changed since the last save.
    static int next_hue_;               ///< Next hue value in the rotating colour sequence (set in .cpp).
    static int starting_hue_;          ///< Initial hue value, used to reset the sequence.

    /*! \brief Generates the next colour in the rotating hue sequence, advances the counter,
     *  and records the index used on this instance.
     *
     * \return A new QColor for the next tag family.
     */
    QColor generateNextColor();

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

    /*! \brief Constructs a TagFamily with the given name and a colour restored from a stored index.
     *
     * Does not advance the shared hue counter, so the sequence is not disturbed when
     * reloading persisted families.
     *
     * \param tf         The family name string.
     * \param colorIndex The hue-sequence index that was used when this family was first created.
     * \param parent     Qt parent object.
     */
    TagFamily(QString tf, int colorIndex, QObject *parent);

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

    /*! \brief Returns the hue-sequence index used to generate this family's colour. */
    int getColorIndex() const;

    /*! \brief Returns the current value of the shared hue counter (i.e. the index the next new family will use). */
    static int getNextHue();

    /*! \brief Sets the shared hue counter, allowing a saved sequence position to be restored. */
    static void setNextHue(int hue);

    /*! \brief Computes the colour for a given hue-sequence index without advancing the counter.
     *
     * \param hueIndex The index to compute a colour for.
     * \return The corresponding QColor.
     */
    static QColor generateColorForIndex(int hueIndex);

    /*! \brief Updates this family's colour to the one corresponding to the given hue-sequence index.
     *
     * Does not advance the shared hue counter. Used when applying a color looked up from a
     * library file to an already-existing TagFamily instance.
     *
     * \param colorIndex The hue-sequence index to apply.
     */
    void setColorFromIndex(int colorIndex);

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
