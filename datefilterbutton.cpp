#include "datefilterbutton.h"

#include <QCalendarWidget>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

/*! \brief Constructs a DateFilterButton.
 *
 * \param parent Optional Qt parent widget.
 */
DateFilterButton::DateFilterButton(QWidget *parent)
    : QWidget(parent)
    , lineEdit_(new QLineEdit(this))
    , calendarButton_(new QToolButton(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(lineEdit_);
    layout->addWidget(calendarButton_);

    lineEdit_->setPlaceholderText("Any date");
    lineEdit_->setFixedWidth(lineEdit_->fontMetrics().horizontalAdvance("00/00/0000") + 12);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    calendarButton_->setText("▾");
    calendarButton_->setFocusPolicy(Qt::NoFocus);

    connect(lineEdit_, &QLineEdit::editingFinished, this, &DateFilterButton::onEditingFinished);
    connect(calendarButton_, &QToolButton::clicked,  this, &DateFilterButton::showCalendar);
}

/*! \brief Returns the currently selected date, or an invalid QDate if none. */
QDate DateFilterButton::date() const
{
    return date_;
}

/*! \brief Validates the typed text and emits dateChanged() when editing finishes. */
void DateFilterButton::onEditingFinished()
{
    QDate parsed = parseDate(lineEdit_->text());
    if (parsed == date_)
        return;
    date_ = parsed;
    // If the text wasn't a valid date, clear the field so the placeholder shows
    if (!date_.isValid())
        lineEdit_->clear();
    emit dateChanged(date_);
}

/*! \brief Opens the calendar popup. */
void DateFilterButton::showCalendar()
{
    QFrame *popup = new QFrame(nullptr, Qt::Popup);
    popup->setAttribute(Qt::WA_DeleteOnClose);

    auto *layout = new QVBoxLayout(popup);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    auto *cal = new QCalendarWidget(popup);
    if (date_.isValid())
        cal->setSelectedDate(date_);

    auto *clearBtn = new QPushButton("Clear", popup);
    layout->addWidget(cal);
    layout->addWidget(clearBtn);

    connect(clearBtn, &QPushButton::clicked, this, [this, popup]() {
        date_ = QDate();
        lineEdit_->clear();
        emit dateChanged(date_);
        popup->close();
    });

    connect(cal, &QCalendarWidget::clicked, this, [this, popup](const QDate &date) {
        date_ = date;
        updateLineEdit();
        emit dateChanged(date_);
        popup->close();
    });

    popup->move(mapToGlobal(QPoint(0, height())));
    popup->show();
}

/*! \brief Attempts to parse \a text as a date using locale and common formats.
 *
 * \param text The raw string from the line edit.
 * \return A valid QDate on success, or an invalid QDate if parsing fails or text is empty.
 */
QDate DateFilterButton::parseDate(const QString &text) const
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return QDate();

    QLocale locale;
    QDate d = locale.toDate(t, QLocale::ShortFormat);
    if (d.isValid()) return d;

    d = locale.toDate(t, QLocale::LongFormat);
    if (d.isValid()) return d;

    for (const QString &fmt : {"yyyy-MM-dd", "dd/MM/yyyy", "MM/dd/yyyy", "d/M/yyyy", "dd.MM.yyyy"})
    {
        d = QDate::fromString(t, fmt);
        if (d.isValid()) return d;
    }

    return QDate();
}

/*! \brief Updates the line edit to display the current date in short locale format. */
void DateFilterButton::updateLineEdit()
{
    lineEdit_->setText(date_.isValid() ? QLocale().toString(date_, QLocale::ShortFormat) : QString());
}
