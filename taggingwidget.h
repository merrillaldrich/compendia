#ifndef TAGGINGWIDGET_H
#define TAGGINGWIDGET_H

#include <QObject>
#include <QWidget>
#include <QColor>

/*! \brief Base widget class that carries a base colour for tag and tag-family widgets.
 *
 * TagWidget and TagFamilyWidget both inherit from this class so that child
 * widgets such as TagWidgetCloseButton can retrieve the parent's colour without
 * a dynamic cast to a specific subclass.
 */
class TaggingWidget : public QWidget
{
    Q_OBJECT

public:
    /*! \brief Constructs a TaggingWidget with no explicit colour set.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit TaggingWidget(QWidget *parent = nullptr);

    /*! \brief Constructs a TaggingWidget with the given base colour.
     *
     * \param color  The colour to store in base_color_.
     * \param parent Qt parent widget.
     */
    TaggingWidget(QColor color, QWidget *parent);

    /*! \brief The base colour used for painting this widget and its close button. */
    QColor base_color_;

};


#endif // TAGGINGWIDGET_H
