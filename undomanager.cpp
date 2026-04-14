#include "undomanager.h"
#include "compendiacore.h"

UndoManager::UndoManager(CompendiaCore* core, QObject* parent)
    : QObject(parent), core_(core) {}

void UndoManager::checkpoint(const QString& description) {
    ModelSnapshot snap = core_->captureSnapshot(description);
    undoStack_.append(snap);
    if (undoStack_.size() > kMaxDepth)
        undoStack_.removeFirst();
    redoStack_.clear();
    notifyStateChanged();
}

void UndoManager::undo() {
    if (!canUndo()) return;
    ModelSnapshot target = undoStack_.takeLast();
    ModelSnapshot current = core_->captureSnapshot(target.description);
    redoStack_.append(current);
    core_->restoreSnapshot(target);
    notifyStateChanged();
}

void UndoManager::redo() {
    if (!canRedo()) return;
    ModelSnapshot target = redoStack_.takeLast();
    ModelSnapshot current = core_->captureSnapshot(target.description);
    undoStack_.append(current);
    core_->restoreSnapshot(target);
    notifyStateChanged();
}

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    notifyStateChanged();
}

QString UndoManager::undoText() const {
    return canUndo() ? undoStack_.last().description : QString();
}

QString UndoManager::redoText() const {
    return canRedo() ? redoStack_.last().description : QString();
}

QAction* UndoManager::createUndoAction(QObject* parent, const QString& prefix) {
    auto* action = new QAction(prefix, parent);
    action->setEnabled(canUndo());
    connect(this, &UndoManager::canUndoChanged, action, &QAction::setEnabled);
    connect(this, &UndoManager::undoTextChanged, action, [action, prefix](const QString& text) {
        action->setText(text.isEmpty() ? prefix : prefix + u" " + text);
    });
    connect(action, &QAction::triggered, this, &UndoManager::undo);
    return action;
}

QAction* UndoManager::createRedoAction(QObject* parent, const QString& prefix) {
    auto* action = new QAction(prefix, parent);
    action->setEnabled(canRedo());
    connect(this, &UndoManager::canRedoChanged, action, &QAction::setEnabled);
    connect(this, &UndoManager::redoTextChanged, action, [action, prefix](const QString& text) {
        action->setText(text.isEmpty() ? prefix : prefix + u" " + text);
    });
    connect(action, &QAction::triggered, this, &UndoManager::redo);
    return action;
}

void UndoManager::notifyStateChanged() {
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
    emit undoTextChanged(undoText());
    emit redoTextChanged(redoText());
}
