#ifndef FOLDERFILTERLINEEDIT_H
#define FOLDERFILTERLINEEDIT_H

#include <QKeyEvent>
#include <QLineEdit>
#include <QList>

/*! \brief A QLineEdit with shell-style path autocomplete for the folder filter.
 *
 * As the user types a path fragment, the widget searches the folder paths of all
 * loaded files and suggests the characters that follow the typed text up to the
 * next path delimiter, picking the alphabetically first candidate.
 *
 * Accepting a suggestion works the same way as shell tab-completion:
 *   - Right arrow or Tab: accept and keep focus in this field
 *   - Enter/Return: accept and keep focus
 *   - Backspace/Delete: dismiss the suggestion
 */
class FolderFilterLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    /*! \brief Constructs a FolderFilterLineEdit.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit FolderFilterLineEdit(QWidget *parent = nullptr);

    /*! \brief Supplies the list of folder paths that autocomplete searches.
     *
     * Call this after every folder load with the unique folder paths of all loaded files.
     * \param paths Folder paths to search; duplicates are ignored internally.
     */
    void setAvailablePaths(const QList<QString> &paths);

protected:
    /*! \brief Intercepts key events to handle completion acceptance and dismissal.
     *
     * \param event The key-press event.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /*! \brief Prevents Tab from moving focus away while a suggestion is active.
     *
     * \param next Direction of focus traversal (unused when returning false).
     * \return False when a suggestion is shown, otherwise delegates to the base class.
     */
    bool focusNextPrevChild(bool next) override;

private slots:
    /*! \brief Performs inline path autocomplete as the user types.
     *
     * \param text The current line edit text as typed by the user.
     */
    void onTextEdited(const QString &text);

private:
    /*! \brief Searches available paths for the best completion for \a typed.
     *
     * Finds every occurrence of \a typed in every available path (case-insensitive),
     * collects the characters that follow up to the next path delimiter, sorts them
     * alphabetically, and returns the first.
     *
     * \param typed The text the user has typed so far.
     * \return The suggested completion suffix, or an empty string if none found.
     */
    QString findCompletion(const QString &typed) const;

    QList<QString> availablePaths_;                    ///< Folder paths available for autocomplete suggestions.
    bool           suppressNextCompletion_ = false;    ///< True to skip one completion cycle after an accept/dismiss.
};

#endif // FOLDERFILTERLINEEDIT_H
