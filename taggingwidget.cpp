#include "taggingwidget.h"

/*! \brief Constructs a TaggingWidget with no explicit colour set.
 *
 * \param parent Optional Qt parent widget.
 */
TaggingWidget::TaggingWidget(QWidget *parent)
    : QWidget{parent}{

}

/*! \brief Constructs a TaggingWidget with the given base colour.
 *
 * \param color  The colour to store in base_color_.
 * \param parent Qt parent widget.
 */
TaggingWidget::TaggingWidget(QColor color, QWidget *parent)
    : QWidget{parent}{

    base_color_ = color;
}
