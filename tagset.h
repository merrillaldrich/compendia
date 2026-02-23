#ifndef TAGSET_H
#define TAGSET_H

#include <optional>
#include <QString>
#include <QRect>

/*! \brief Lightweight value type pairing a tag-family name with a tag name,
 *         with an optional bounding rectangle within the image.
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
    std::optional<QRect> rect;

    /*! \brief Initialises the tag-set with the given family and tag name strings.
     *
     * \param tfn The tag-family name.
     * \param tn  The tag name.
     */
    TagSet(QString tfn, QString tn);

    /*! \brief Initialises the tag-set with a family name, tag name, and a bounding rectangle.
     *
     * \param tfn  The tag-family name.
     * \param tn   The tag name.
     * \param rect The bounding rectangle within the image.
     */
    TagSet(QString tfn, QString tn, QRect rect);

signals:
};

#endif // TAGSET_H
