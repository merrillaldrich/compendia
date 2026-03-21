#ifndef FILENAMEDELEGATE_H
#define FILENAMEDELEGATE_H

#include <QStyledItemDelegate>
#include <QFontMetrics>
#include <QStringList>

class CompendiaCore;

/*! \brief Item delegate that renders filenames as wrapped multi-line text beneath each icon.
 *
 * The model data (Qt::DisplayRole) is never modified; wrapping is computed at
 * paint time from the available cell width.  Natural break points (_  -  .  space)
 * are preferred; character-level hard breaks are used only when a single token
 * exceeds the cell width on its own.  Output is capped at three lines, with an
 * ellipsis appended if the full name does not fit.
 *
 * The delegate looks up icons from CompendiaCore::iconForPath() rather than from
 * Qt::DecorationRole, so that the LRU icon pool in CompendiaCore remains the sole
 * source of truth for loaded thumbnails.
 */
class FileNameDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /*! \brief Constructs a FileNameDelegate.
     *
     * \param core   CompendiaCore instance used for on-demand icon lookup.
     * \param parent Optional Qt parent object.
     */
    explicit FileNameDelegate(CompendiaCore *core, QObject *parent = nullptr);

    /*! \brief Paints an item: icon via the base class, then wrapped filename text below it.
     *
     * \param painter The painter to draw into.
     * \param option  Style and geometry information for the item.
     * \param index   Model index of the item being drawn.
     */
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    /*! \brief Returns the preferred size for an item given the current icon and font.
     *
     * \param option Style and geometry information.
     * \param index  Model index of the item.
     * \return Recommended QSize for the item cell.
     */
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    /*! \brief Breaks \a name into display lines that each fit within \a maxWidth pixels.
     *
     * Tokenises the name at natural delimiters (_ - . space), packs tokens
     * greedily onto lines, hard-breaks tokens that are individually too wide,
     * and caps the result at three lines (appending … on the third if needed).
     *
     * \param name     The filename string to wrap.
     * \param fm       Font metrics used to measure rendered widths.
     * \param maxWidth Maximum pixel width of a single line.
     * \return Ordered list of display lines (at most three entries).
     */
    static QStringList wrapFileName(const QString &name,
                                    const QFontMetrics &fm,
                                    int maxWidth);

private:
    CompendiaCore *core_ = nullptr; ///< CompendiaCore used to look up cached icons by file path.

protected:
    /*! \brief Populates \a option from the model and then clears the text field.
     *
     * Overriding this prevents the base-class paint path from drawing any text at
     * all — the text is drawn manually in paint() instead.  Both paint() and
     * sizeHint() read the display string directly from the index rather than from
     * the option, so they are unaffected by the cleared field.
     *
     * \param option The style option to populate.
     * \param index  Model index of the item.
     */
    void initStyleOption(QStyleOptionViewItem *option,
                         const QModelIndex &index) const override;
};

#endif // FILENAMEDELEGATE_H
