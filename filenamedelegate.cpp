#include "filenamedelegate.h"
#include "luminismcore.h"
#include "taggedfile.h"
#include <QPainter>
#include <QApplication>

/*! \brief Constructs a FileNameDelegate.
 *
 * \param parent Optional Qt parent object.
 */
FileNameDelegate::FileNameDelegate(LuminismCore *core, QObject *parent)
    : QStyledItemDelegate(parent)
    , core_(core)
{
}

/*! \brief Breaks \a name into display lines that each fit within \a maxWidth pixels.
 *
 * Tokenises at natural delimiters (_ - . space), keeping each delimiter
 * attached to the token that precedes it (so "IMG_2024" → ["IMG_", "2024"]).
 * Tokens are then packed greedily onto lines.  A token that is individually
 * wider than \a maxWidth is split character by character.  The result is
 * capped at three lines; if the name does not fit, the third line gets an
 * ellipsis appended.
 *
 * \param name     The filename string to wrap.
 * \param fm       Font metrics used to measure rendered widths.
 * \param maxWidth Maximum pixel width of a single line.
 * \return Ordered list of display lines (at most three entries).
 */
QStringList FileNameDelegate::wrapFileName(const QString &name,
                                           const QFontMetrics &fm,
                                           int maxWidth)
{
    if (maxWidth <= 0 || name.isEmpty())
        return {name};

    // --- Tokenise at natural break characters, delimiter stays with its token ---
    QStringList tokens;
    QString cur;
    for (const QChar c : name) {
        cur += c;
        if (c == QLatin1Char('_') || c == QLatin1Char('-') ||
            c == QLatin1Char('.')  || c == QLatin1Char(' ')) {
            tokens.append(cur);
            cur.clear();
        }
    }
    if (!cur.isEmpty())
        tokens.append(cur);

    // --- Greedy line packing with character-level hard-break fallback ---
    QStringList lines;
    QString line;

    // Push any complete lines and leave the last fitting chunk in `line`.
    auto hardBreak = [&](QString &text) {
        while (fm.horizontalAdvance(text) > maxWidth && text.size() > 1) {
            // Linear scan from the right to find the last char that still fits.
            int n = text.size() - 1;
            while (n > 1 && fm.horizontalAdvance(text.left(n)) > maxWidth)
                --n;
            lines.append(text.left(n));
            text = text.mid(n);
        }
    };

    for (const QString &token : tokens) {
        const QString candidate = line + token;
        if (fm.horizontalAdvance(candidate) <= maxWidth) {
            line = candidate;
        } else {
            // Current line is full — flush it and start a new one with this token.
            if (!line.isEmpty()) {
                lines.append(line);
                line.clear();
            }
            line = token;
            hardBreak(line); // splits oversized token; last fitting chunk stays in line
        }
    }
    if (!line.isEmpty())
        lines.append(line);

    // --- Cap at 3 lines, appending ellipsis if truncated ---
    const int maxLines = 3;
    if (lines.size() > maxLines) {
        lines = lines.mid(0, maxLines);
        QString &last = lines.last();
        const QChar ellipsis(0x2026); // …
        while (last.size() > 1 && fm.horizontalAdvance(last + ellipsis) > maxWidth)
            last.chop(1);
        last += ellipsis;
    }

    return lines;
}

/*! \brief Populates \a option from the model and then clears the text field.
 *
 * QStyledItemDelegate::paint() calls initStyleOption() as a virtual call
 * internally, so overriding it here is the only reliable way to prevent the
 * base class from drawing the filename as a single truncated line.
 *
 * \param option The style option to populate.
 * \param index  Model index of the item.
 */
void FileNameDelegate::initStyleOption(QStyleOptionViewItem *option,
                                        const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->text.clear();

    // Inject the LRU-pool icon here so it survives QStyledItemDelegate::paint()'s
    // internal initStyleOption() call, which would otherwise overwrite any icon
    // we set on opt between initStyleOption and paint in paint().
    if (core_) {
        TaggedFile *tf = index.data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf) {
            QIcon poolIcon = core_->iconForPath(tf->filePath + "/" + tf->fileName);
            if (!poolIcon.isNull())
                option->icon = poolIcon;
        }
    }
}

/*! \brief Paints an item: icon via the base class, then wrapped filename text below it.
 *
 * The base class is called via initStyleOption (which clears the text), so it
 * draws only the selection background and the icon.  Wrapped text lines are
 * then drawn manually, centred horizontally in the cell below the icon area.
 *
 * \param painter The painter to draw into.
 * \param option  Style and geometry information for the item.
 * \param index   Model index of the item being drawn.
 */
void FileNameDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    // Read display text from the index directly — initStyleOption clears opt.text.
    const QString displayText = index.data(Qt::DisplayRole).toString();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyledItemDelegate::paint(painter, opt, index); // draws background + icon; no text

    // --- Draw wrapped filename below the icon ---
    const QRect &r = opt.rect;
    const int textTop   = r.top() + opt.decorationSize.height() + 4;
    const int textWidth = r.width() - 4;

    const QFontMetrics fm(opt.font);
    const QStringList lines = wrapFileName(displayText, fm, textWidth);

    painter->save();
    painter->setFont(opt.font);
    painter->setPen(opt.palette.text().color());

    int y = textTop;
    for (const QString &line : lines) {
        painter->drawText(QRect(r.left() + 2, y, textWidth, fm.lineSpacing()),
                          Qt::AlignHCenter | Qt::AlignTop,
                          line);
        y += fm.lineSpacing();
        if (y >= r.bottom())
            break;
    }
    painter->restore();
}

/*! \brief Returns the preferred size for an item given the current icon and font.
 *
 * Height is icon height + 4 px gap + up to three wrapped text lines + 4 px
 * bottom padding.  This keeps the hint consistent with the grid size formula
 * used in MainWindow.
 *
 * \param option Style and geometry information.
 * \param index  Model index of the item.
 * \return Recommended QSize for the item cell.
 */
QSize FileNameDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    // Read display text from the index directly — initStyleOption clears opt.text.
    const QString displayText = index.data(Qt::DisplayRole).toString();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const int cellWidth  = opt.decorationSize.width() + 20;
    const int textWidth  = cellWidth - 4;
    const QFontMetrics fm(opt.font);
    const QStringList lines = wrapFileName(displayText, fm, textWidth);

    const int textH  = lines.count() * fm.lineSpacing();
    const int totalH = opt.decorationSize.height() + 4 + textH + 4;
    return QSize(cellWidth, totalH);
}
