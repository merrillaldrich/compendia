#include "tagset.h"

/*! \brief Initialises the tag-set with the given family and tag name strings.
 *
 * \param tfn The tag-family name.
 * \param tn  The tag name.
 */
TagSet::TagSet(QString tfn, QString tn){
    tagFamilyName = tfn;
    tagName = tn;
}

/*! \brief Initialises the tag-set with a family name, tag name, and a bounding rectangle.
 *
 * \param tfn  The tag-family name.
 * \param tn   The tag name.
 * \param r    The bounding rectangle within the image.
 */
TagSet::TagSet(QString tfn, QString tn, QRect r){
    tagFamilyName = tfn;
    tagName = tn;
    rect = r;
}
