#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <QObject>
#include <QAction>
#include <QList>
#include <QSet>
#include <QMap>
#include <QRectF>
#include <QString>
#include <optional>

class CompendiaCore;

/*! \brief An in-memory snapshot of all mutable model state.
 *
 * Captures the tag library (families + tags + colour indices) and
 * per-file tag assignments and ratings.  EXIF data, pHash values,
 * file paths, and filter state are excluded — they are either
 * read-only or view state and do not need to be undone.
 */
struct ModelSnapshot {
    QString description;   ///< Human-readable label for the action that produced this state.
    int nextColorIndex = 0;

    struct FamilySnap {
        QString name;
        int colorIndex = 0;
    };
    struct TagSnap {
        QString familyName;
        QString tagName;
    };
    /*! \brief Per-file state captured in the snapshot. */
    struct FileSnap {
        QString key;                         ///< filePath + "/" + fileName
        QSet<QString> tagKeys;               ///< "FamilyName\tTagName" for each applied tag
        QMap<QString, QRectF> tagRects;      ///< same key -> normalised bounding rect
        std::optional<int> rating;
    };

    QList<FamilySnap> families;
    QList<TagSnap>    tags;
    QList<FileSnap>   files;
};

/*! \brief Two-stack undo/redo manager using full model snapshots.
 *
 * Before each user mutation, call checkpoint() to push the current model
 * state onto the undo stack.  undo() pops the most recent snapshot and
 * restores it; redo() reapplies the last undone state.  New checkpoints
 * clear the redo stack.
 *
 * Exposes QAction helpers compatible with the standard Edit-menu pattern.
 */
class UndoManager : public QObject {
    Q_OBJECT

public:
    explicit UndoManager(CompendiaCore* core, QObject* parent = nullptr);

    /*! \brief Captures the current model state and pushes it onto the undo stack.
     *
     * Call this BEFORE executing a user mutation.  The redo stack is cleared.
     * \param description Human-readable label shown in the Undo action text.
     */
    void checkpoint(const QString& description);

    void undo();
    void redo();

    /*! \brief Clears both undo and redo stacks.
     *
     * Used after irreversible operations such as merge.
     */
    void clear();

    bool canUndo() const { return !undoStack_.isEmpty(); }
    bool canRedo() const { return !redoStack_.isEmpty(); }

    QString undoText() const;
    QString redoText() const;

    /*! \brief Creates a Undo QAction whose enabled state and label track the stack.
     *
     * \param parent  Qt parent for the returned action.
     * \param prefix  Text prepended to the action label, e.g. "Undo".
     */
    QAction* createUndoAction(QObject* parent, const QString& prefix = tr("Undo"));
    /*! \brief Creates a Redo QAction. */
    QAction* createRedoAction(QObject* parent, const QString& prefix = tr("Redo"));

signals:
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void undoTextChanged(const QString& text);
    void redoTextChanged(const QString& text);

private:
    CompendiaCore* core_;
    QList<ModelSnapshot> undoStack_;   ///< Pre-mutation states; most recent is last.
    QList<ModelSnapshot> redoStack_;   ///< States available to redo; most recent is last.
    static constexpr int kMaxDepth = 20;

    void notifyStateChanged();
};

#endif // UNDOMANAGER_H
