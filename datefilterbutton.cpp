#include "datefilterbutton.h"

#include <algorithm>

#include <QCalendarWidget>
#include <QDate>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
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
    lineEdit_->setFixedHeight(24);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    calendarButton_->setText("▾");
    calendarButton_->setFocusPolicy(Qt::NoFocus);
    calendarButton_->setFixedHeight(24);

    lineEdit_->installEventFilter(this);

    connect(lineEdit_, &QLineEdit::editingFinished, this, &DateFilterButton::onEditingFinished);
    connect(lineEdit_, &QLineEdit::textEdited,      this, &DateFilterButton::onTextEdited);
    connect(calendarButton_, &QToolButton::clicked,  this, &DateFilterButton::showCalendar);
}

/*! \brief Returns the currently selected date, or an invalid QDate if none. */
QDate DateFilterButton::date() const
{
    return date_;
}

/*! \brief Clears the date filter and emits dateChanged() with an invalid date. */
void DateFilterButton::clear()
{
    if (!date_.isValid())
        return;
    date_ = QDate();
    lineEdit_->clear();
    emit dateChanged(date_);
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
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    // Apply stylesheet to remove the month menu arrow b/c it looks sloppy
    cal->setStyleSheet(
        "QToolButton#qt_calendar_monthbutton::menu-indicator {"
        "    image: none;"   // Remove arrow image
        "    width: 0px;"    // Remove space for arrow
        "}"
    );

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

/*! \brief Supplies the sorted list of dates that autocomplete draws from.
 *
 * \param dates The available dates; will be sorted chronologically internally.
 */
void DateFilterButton::setAvailableDates(const QList<QDate> &dates)
{
    availableDates_ = dates;
    std::sort(availableDates_.begin(), availableDates_.end());
}

/*! \brief Performs inline autocomplete as the user types.
 *
 * Searches availableDates_ for the first chronological match and shows the
 * remaining characters as a selected suffix.  Backspace/Delete on the
 * completion dismisses it without re-triggering autocomplete.
 * \param text The current line edit text as typed by the user.
 */
void DateFilterButton::onTextEdited(const QString &text)
{
    if (suppressNextCompletion_) {
        suppressNextCompletion_ = false;
        return;
    }

    if (text.isEmpty() || availableDates_.isEmpty())
        return;

    QLocale locale;
    for (const QDate &d : availableDates_) {
        QString formatted = locale.toString(d, QLocale::ShortFormat);
        if (formatted.startsWith(text, Qt::CaseInsensitive)) {
            lineEdit_->setText(formatted);
            lineEdit_->setSelection(text.length(), formatted.length() - text.length());
            return;
        }
    }
}

/*! \brief Intercepts key events on the line edit to handle completion acceptance and dismissal.
 *
 * \param obj   The watched object.
 * \param event The incoming event.
 * \return True if the event was consumed, false to let it propagate.
 */
bool DateFilterButton::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == lineEdit_ && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);

        if (lineEdit_->hasSelectedText()) {
            if (ke->key() == Qt::Key_Backspace || ke->key() == Qt::Key_Delete) {
                // Dismiss the completion without re-triggering autocomplete
                suppressNextCompletion_ = true;
                lineEdit_->setText(lineEdit_->text().left(lineEdit_->selectionStart()));
                return true;
            }
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                // Accept completion: deselect and move cursor to end, keep focus here.
                // Let the event propagate so editingFinished fires and applies the filter.
                lineEdit_->setCursorPosition(lineEdit_->text().length());
                return false;
            }
            if (ke->key() == Qt::Key_Tab) {
                // Accept completion: move cursor to end, then let Tab change focus normally
                lineEdit_->setCursorPosition(lineEdit_->text().length());
                return false;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
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
