#ifndef TAGSET_H
#define TAGSET_H

#include <QString>

/*! \brief Lightweight value type pairing a tag-family name with a tag name.
 *
 * Used primarily when parsing JSON sidecar files and when passing tag
 * identity through drag-and-drop operations before full Tag objects are
 * resolved.
 */
class TagSet
{
public:
    QString tagFamilyName = "";
    QString tagName = "";

    /*! \brief Initialises the tag-set with the given family and tag name strings.
     *
     * \param tfn The tag-family name.
     * \param tn  The tag name.
     */
    TagSet(QString tfn, QString tn);

signals:
};

#endif // TAGSET_H
