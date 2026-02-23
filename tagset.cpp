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
