#ifndef DATEFILTERBUTTON_H
#define DATEFILTERBUTTON_H

#include <QDate>
#include <QList>
#include <QWidget>

class QLineEdit;
class QToolButton;

/*! \brief A date-entry widget combining a free-form line edit with a calendar popup button.
 *
 * The user may type a date in any common format; the value is parsed and validated when
 * editing finishes (focus-out or Return).  A small button beside the line edit opens a
 * QCalendarWidget popup; selecting a date there fills the line edit.  An empty line edit
 * means "no filter" and emits dateChanged() with an invalid QDate.
 *
 * While the user types, the widget provides inline autocomplete: it searches the set of
 * dates supplied via setAvailableDates() and shows the first chronological match as a
 * selected (highlighted) suffix after the cursor.  Pressing Right, Tab, or Enter accepts
 * the suggestion; Backspace or Delete dismisses it.
 */
class DateFilterButton : public QWidget
{
    Q_OBJECT

public:
    /*! \brief Constructs a DateFilterButton.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit DateFilterButton(QWidget *parent = nullptr);

    /*! \brief Returns the currently selected date, or an invalid QDate if none. */
    QDate date() const;

    /*! \brief Supplies the sorted list of dates that autocomplete draws from.
     *
     * Call this after every folder load.  Dates should correspond to the effective
     * date of each file (EXIF capture date when present, filesystem creation date otherwise).
     * \param dates The available dates; will be sorted chronologically internally.
     */
    void setAvailableDates(const QList<QDate> &dates);

signals:
    /*! \brief Emitted when the selected date changes.
     *
     * \param date The new date, or an invalid QDate if the field was cleared.
     */
    void dateChanged(QDate date);

private slots:
    /*! \brief Validates the typed text and emits dateChanged() when editing finishes. */
    void onEditingFinished();

    /*! \brief Performs inline autocomplete as the user types. */
    void onTextEdited(const QString &text);

    /*! \brief Opens the calendar popup. */
    void showCalendar();

protected:
    /*! \brief Intercepts key events on the line edit to handle completion acceptance and dismissal.
     *
     * \param obj   The watched object.
     * \param event The incoming event.
     * \return True if the event was consumed, false to let it propagate.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    /*! \brief Attempts to parse \a text as a date using locale and common formats.
     *
     * \param text The raw string from the line edit.
     * \return A valid QDate on success, or an invalid QDate if parsing fails or text is empty.
     */
    QDate parseDate(const QString &text) const;

    /*! \brief Updates the line edit to display the current date in short locale format. */
    void updateLineEdit();

    QLineEdit   *lineEdit_;
    QToolButton *calendarButton_;
    QDate        date_;

    QList<QDate> availableDates_;
    bool         suppressNextCompletion_ = false;
};

#endif // DATEFILTERBUTTON_H
