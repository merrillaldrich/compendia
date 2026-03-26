#include "tagfamily.h"

// Single static variable to hold the hue (color wheel angle) of the last
// used dynamic tag color
int TagFamily::starting_hue_ = 20;
int TagFamily::next_hue_ = starting_hue_;

/*! \brief Constructs a TagFamily with an empty name and an auto-assigned colour.
 *
 * \param parent Optional Qt parent object.
 */
TagFamily::TagFamily(QObject *parent)
    : QObject{parent}{
    tag_family_name_ = "";
    tag_family_color_ = TagFamily::generateNextColor();
}

/*! \brief Constructs a TagFamily with the given name and an auto-assigned colour.
 *
 * \param tf     The family name string.
 * \param parent Qt parent object.
 */
TagFamily::TagFamily(QString tf, QObject *parent = nullptr)
    : QObject{parent}{
    tag_family_name_ = tf;
    tag_family_color_ = TagFamily::generateNextColor();
}

/*! \brief Constructs a TagFamily with the given name and a colour restored from a stored index.
 *
 * Does not advance the shared hue counter.
 *
 * \param tf         The family name string.
 * \param colorIndex The hue-sequence index that was used when this family was first created.
 * \param parent     Qt parent object.
 */
TagFamily::TagFamily(QString tf, int colorIndex, QObject *parent)
    : QObject{parent}{
    tag_family_name_ = tf;
    color_index_ = colorIndex;
    tag_family_color_ = TagFamily::generateColorForIndex(colorIndex);
}

/*! \brief Sets the family name and marks the dirty flag if the name changed.
 *
 * \param tagFamilyName The new name for this family.
 */
void TagFamily::setName(QString tagFamilyName){
    if(tag_family_name_ != tagFamilyName){
        tag_family_name_ = tagFamilyName;
        dirty_flag_ = true;
        emit nameChanged();
    }
}

/*! \brief Returns whether the family name has been modified since the last save.
 *
 * \return True if the dirty flag is set.
 */
bool TagFamily::dirtyFlag() const
{
    return dirty_flag_;
}

/*! \brief Clears the dirty flag after changes have been persisted. */
void TagFamily::clearDirtyFlag()
{
    dirty_flag_ = false;
}

/*! \brief Returns the current family name.
 *
 * \return The family name string.
 */
QString TagFamily::getName() const {
    return tag_family_name_;
}

/*! \brief Returns the colour assigned to this family.
 *
 * \return The family colour.
 */
QColor TagFamily::getColor() const {
    return tag_family_color_;
}

/*! \brief Serialises a TagFamily's name to a QDataStream for drag-and-drop transfer.
 *
 * \param out The output data stream.
 * \param t   The TagFamily to serialise.
 * \return The output stream after writing.
 */
QDataStream &operator<<(QDataStream &out, const TagFamily &t) {
    QString name = t.getName();
    out << name;
    return out;
}

/*! \brief Deserialises a TagFamily's name from a QDataStream during drag-and-drop.
 *
 * \param in The input data stream.
 * \param t  The TagFamily to populate with the read name.
 * \return The input stream after reading.
 */
QDataStream &operator>>(QDataStream &in, TagFamily &t) {
    QString newName;
    in >> newName;
    t.setName(newName);
    return in;
}

/*! \brief Computes the colour for a given hue-sequence index without advancing the counter.
 *
 * \param hueIndex The index to compute a colour for.
 * \return The corresponding QColor.
 */
QColor TagFamily::generateColorForIndex(int hueIndex){
    int current_hue = hueIndex % 360;
    double times_around_the_wheel = hueIndex / 360.0;
    int default_saturation = 190;
    int default_value = 220;

    int current_saturation = default_saturation - (default_saturation * times_around_the_wheel * 0.2);
    int current_value = default_value - (default_value * times_around_the_wheel * 0.07);

    QColor color = QColor();
    color.setHsv(current_hue, current_saturation, current_value);
    return color;
}

/*! \brief Generates the next colour in the rotating hue sequence and advances the counter.
 *
 * Steps the hue by a fixed increment each call, skipping quickly through
 * the green range where human colour discrimination is lower.
 *
 * \return A new QColor for the next tag family.
 */
QColor TagFamily::generateNextColor(){
    color_index_ = next_hue_;

    QColor color = generateColorForIndex(next_hue_);

    next_hue_ += 32;

    // move faster across greens since people can't perceive the differences
    // as well as other colors

    if (next_hue_ % 360 > 95 && next_hue_ % 360 < 175)
        next_hue_ += 7;

    return color;
}

/*! \brief Returns the hue-sequence index used to generate this family's colour. */
int TagFamily::getColorIndex() const {
    return color_index_;
}

/*! \brief Returns the current value of the shared hue counter. */
int TagFamily::getNextHue() {
    return next_hue_;
}

/*! \brief Sets the shared hue counter, restoring a saved sequence position. */
void TagFamily::setNextHue(int hue) {
    next_hue_ = hue;
}

/*! \brief Resets the colour-generation sequence back to the starting hue. */
void TagFamily::restartColorSequence(){
    next_hue_ = starting_hue_;
}
