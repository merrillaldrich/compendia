#include "folderfilterlineedit.h"

/*! \brief Constructs a FolderFilterLineEdit.
 *
 * \param parent Optional Qt parent widget.
 */
FolderFilterLineEdit::FolderFilterLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    connect(this, &QLineEdit::textEdited, this, &FolderFilterLineEdit::onTextEdited);
}

/*! \brief Supplies the list of folder paths that autocomplete searches.
 *
 * \param paths Folder paths to search; duplicates are ignored internally.
 */
void FolderFilterLineEdit::setAvailablePaths(const QList<QString> &paths)
{
    availablePaths_ = paths;
}

/*! \brief Intercepts key events to handle completion acceptance and dismissal.
 *
 * \param event The key-press event.
 */
void FolderFilterLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        // Never suggest after moving backwards
        suppressNextCompletion_ = true;
        if (hasSelectedText()) {
            // Dismiss the active suggestion
            setText(text().left(selectionStart()));
            event->accept();
            return;
        }
        QLineEdit::keyPressEvent(event);
        return;
    }

    if (hasSelectedText()) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            // Accept: move cursor to end, keep focus, let editingFinished fire normally
            setCursorPosition(text().length());
            QLineEdit::keyPressEvent(event);
            return;
        }
        if (event->key() == Qt::Key_Tab) {
            // Accept: move cursor to end and stay in this field
            setCursorPosition(text().length());
            event->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(event);
}

/*! \brief Prevents Tab from moving focus away while a suggestion is active.
 *
 * \param next Direction of focus traversal (unused when returning false).
 * \return False when a suggestion is shown, otherwise delegates to the base class.
 */
bool FolderFilterLineEdit::focusNextPrevChild(bool next)
{
    if (hasSelectedText())
        return false;
    return QLineEdit::focusNextPrevChild(next);
}

/*! \brief Performs inline path autocomplete as the user types.
 *
 * \param text The current line edit text as typed by the user.
 */
void FolderFilterLineEdit::onTextEdited(const QString &text)
{
    if (suppressNextCompletion_) {
        suppressNextCompletion_ = false;
        return;
    }

    if (text.isEmpty() || availablePaths_.isEmpty())
        return;

    QString completion = findCompletion(text);
    if (completion.isEmpty())
        return;

    setText(text + completion);
    setSelection(text.length(), completion.length());
}

/*! \brief Searches available paths for the best completion for \a typed.
 *
 * \param typed The text the user has typed so far.
 * \return The suggested completion suffix, or an empty string if none found.
 */
QString FolderFilterLineEdit::findCompletion(const QString &typed) const
{
    QStringList completions;

    for (const QString &path : availablePaths_) {
        int searchFrom = 0;
        while (true) {
            int pos = path.indexOf(typed, searchFrom, Qt::CaseInsensitive);
            if (pos == -1) break;

            int afterTyped = pos + typed.length();

            // Find the next path delimiter after the match
            int nextDelim = path.length();
            for (int i = afterTyped; i < path.length(); ++i) {
                if (path[i] == QLatin1Char('/') || path[i] == QLatin1Char('\\')) {
                    nextDelim = i;
                    break;
                }
            }

            if (afterTyped < nextDelim)
                completions.append(path.mid(afterTyped, nextDelim - afterTyped));

            searchFrom = pos + 1;
        }
    }

    if (completions.isEmpty())
        return QString();

    completions.removeDuplicates();
    completions.sort(Qt::CaseInsensitive);
    return completions.first();
}
