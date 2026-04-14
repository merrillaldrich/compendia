#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QItemSelection>
#include <QResizeEvent>
#include <QGraphicsView>
#include <QImageReader>
#include <QMessageBox>
#include <QTimer>
#include "compendiacore.h"
#include "multiprogressbar.h"
#include "facerecognizer.h"
#include "facerecognitionsettingsdialog.h"
#include "framegrabber.h"
#include "constants.h"
#include "aboutdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/*! \brief The application's main window, wiring UI widgets to CompendiaCore operations.
 *
 * MainWindow owns the generated Ui::MainWindow and a CompendiaCore instance.
 * It does not own any file or tag data directly; all data lives in core.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;              ///< Pointer to the generated UI object for this window.
    MultiProgressBar* progress_;     ///< Multi-channel progress bar shown in the status bar.
    QLabel* folderStatsLabel_; ///< Persistent left-side status bar label showing folder summary stats.
    QWidget* welcome_widget_; ///< Shown in the file-list area before any folder is loaded.
    FaceRecognizer* face_recognizer_ = nullptr; ///< Lazily constructed face recognizer; nullptr until first use.
    FrameGrabber* frameGrabber_ = nullptr; ///< Active frame-grab batch, or nullptr when idle.
    QString drillCeilingPath_; ///< Root path the user explicitly opened; drill-up cannot exceed this.
    QTimer* rectWarmupTimer_ = nullptr;       ///< Debounce timer for post-rect-adjust face cache warming.
    TaggedFile* warmupPendingFile_ = nullptr; ///< File waiting for rect-adjust warmup when the timer fires.
    bool preReleaseWarningAccepted_ = false;  ///< True once the user has accepted the pre-release warning this session.

public:
    /*! \brief Constructs the main window, sets up layouts, status bar, and default pane sizes.
     *
     * \param parent Optional Qt parent widget.
     */
    MainWindow(QWidget *parent = nullptr);

    /*! \brief Destroys the main window and releases owned UI and core resources. */
    ~MainWindow();

    /*! \brief The central application controller; provides access to data and business logic. */
    CompendiaCore *core;

    /*! \brief Opens a folder-picker dialog and loads the selected folder into core. */
    void setRootFolder();

    /*! \brief Prompts for confirmation then moves \a tag to \a newFamily via core->refamilyTag(). */
    void onTagRefamilyRequested(Tag* tag, TagFamily* newFamily);

    /*! \brief Updates the file-count label to show visible vs. total file counts. */
    void updateFileCountLabel();

    /*! \brief Updates the persistent folder-stats label in the status bar. */
    void updateFolderStatsLabel();

    /*! \brief Connects the file-count label to the current proxy model's signals.
     *
     * Must be called after every folder load because loadRootDirectory() recreates
     * the proxy model, invalidating any previously established connections.
     */
    void connectFileCountLabel();

    /*! \brief Refreshes the preview pane to reflect the current viewport size. */
    void freshenPreview();

    /*! \brief Clears the preview pane of any displayed image. */
    void clearPreview();

    /*! \brief Rebuilds the tag-library navigation area from the current library tags. */
    void refreshNavTagLibrary();

    /*! \brief Rebuilds the tag-assignment area from tags on the currently filtered files. */
    void refreshTagAssignmentArea();

    /*! \brief Rebuilds the tag-filter area from the currently active filter tags. */
    void refreshTagFilterArea();

private:
    /*! \brief Shows the pre-release warning dialog and returns \c true if the user accepted.
     *
     * Only called when Compendia::ShowPreReleaseWarning is \c true and the warning has not
     * yet been accepted this session.  Sets preReleaseWarningAccepted_ on success.
     * \return \c true if loading should proceed; \c false if the user cancelled.
     */
    bool confirmPreRelease();

    /*! \brief Checks whether a compendiacache folder exists in \p folder and, if not,
     *         prompts the user for confirmation before creating one.
     *
     * \param folder The root folder the user is about to load.
     * \return \c true if loading should proceed; \c false if the user cancelled.
     */
    bool confirmCacheFolder(const QString &folder);

    /*! \brief Lazily constructs face_recognizer_ and loads the DNN model files.
     *
     * Resolves the models/ directory from the application's executable directory,
     * shows a QMessageBox with download instructions if the model files are absent.
     *
     * \return \c true if the recognizer is ready; \c false if model loading failed.
     */
    bool ensureFaceRecognizerLoaded();

    /*! \brief Validates that \p folder is a non-empty, existing, readable directory.
     *
     * Shows a warning dialog describing the problem if validation fails.
     * Returns \c false silently (no dialog) when \p folder is empty, so that
     * a cancelled file-picker dialog does not produce a spurious error message.
     *
     * \param folder The path to validate.
     * \return \c true if the path is safe to pass to core; \c false otherwise.
     */
    bool validateFolder(const QString &folder);

    /*! \brief Rebuilds the previewFileTagsValue label from the currently selected file's tags.
     *
     * Call whenever tag assignments on the previewed file change (drop, replace, delete,
     * face detection). Does nothing if no file is selected.
     */
    void refreshPreviewTagsLabel();

    /*! \brief Validates \p folder, confirms cache creation if needed, then performs
     *         a full load: clears existing state, loads files into core, and refreshes
     *         all UI areas. Both the Browse and Return-key paths delegate here.
     *
     * \param folder Absolute path to the media folder to load.
     */
    void loadFolder(const QString &folder, bool skipCacheConfirm = false);

    /*! \brief Returns true when the current root equals the drill ceiling (or no drill session is active). */
    bool isAtDrillCeiling() const;

    /*! \brief Updates the enabled state of actionDrillUp based on current drill position. */
    void updateDrillUpEnabled();

    /*! \brief Prompts the user to save unsaved changes before a destructive navigation.
     *  \return true if it is safe to proceed (saved or discarded), false if cancelled. */
    bool confirmProceedWithUnsavedChanges();

    /*! \brief Rebuilds the tag-rect overlay list for \p tf and pushes it to the preview.
     *
     * Uses the "Show tagged regions" checkbox state for visibility unless
     * \p forceVisible is true (used after a face sweep, which forces the checkbox on).
     *
     * \param tf           The file whose tag rects should be displayed (non-const; TaggedFile::tags() is not const).
     * \param forceVisible If true, overlays are shown regardless of the checkbox state.
     */
    void refreshPreviewTagRects(TaggedFile* tf, bool forceVisible = false);

    /*! \brief Starts the Save progress bar and writes metadata for all dirty files. */
    void saveAll();

    /*! \brief Sets the folder filter to \p folderPath and enables both clear-isolation actions. */
    void applyFolderIsolation(const QString& folderPath);

    /*! \brief Clears the selection isolation set and disables its clear action. */
    void clearSelectionIsolation();

    /*! \brief Clears the folder filter and disables its clear action. */
    void clearFolderIsolation();

    /*! \brief Guards dirty state, sets drill ceiling if needed, and loads \p targetPath as the new root. */
    void drillToFolder(const QString& targetPath);

    /*! \brief Tags each group of similar files with "Set NN" in the "Similarity Sets" family.
     *
     * Creates or reuses tags named "Set 01", "Set 02", … (zero-padded to at least 2 digits,
     * widening automatically for 100+ groups) in the "Similarity Sets" tag family, then applies
     * the appropriate tag to every file in each group.  Refreshes the tag library and assignment
     * panels when done.
     *
     * \param groups List of file groups as returned by findSimilarImages() or groupBySimilarity().
     */
    void applySimilaritySetTags(const QList<QList<TaggedFile*>> &groups);

    /*! \brief Moves the file list selection forward or backward by \p delta rows.
     *
     * Clamps at the first and last visible row. If multiple files are selected,
     * uses the current (focused) item as the starting point and clears the
     * multi-selection before moving. Does nothing when the list is empty.
     *
     * \param delta +1 to advance to the next file, -1 to go to the previous.
     */
    void navigatePreview(int delta);

private slots:

    /*! \brief Adjusts the file-list icon and grid size when the zoom slider moves.
     *
     * \param value The new slider position (0–100).
     */
    void on_iconZoomSlider_valueChanged(int value);

    /*! \brief Slot for the Open Folder menu action; delegates to setRootFolder(). */
    void on_actionOpen_Folder_triggered();

    /*! \brief Slot for the Quit menu action; closes the main window. */
    void on_actionQuit_triggered();

    /*! \brief Updates the preview and property panel when the file-list selection changes.
     *
     * \param selected   The newly selected model indexes.
     * \param deselected The previously selected model indexes that are now deselected.
     */
    void onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    /*! \brief Refreshes the preview when the preview splitter is moved.
     *
     * \param pos   New splitter handle position.
     * \param index Index of the splitter handle that moved.
     */
    void on_previewSplitter_splitterMoved(int pos, int index);

    /*! \brief Refreshes the preview when the window body splitter is moved.
     *
     * \param pos   New splitter handle position.
     * \param index Index of the splitter handle that moved.
     */
    void on_windowBodySplitter_splitterMoved(int pos, int index);

    /*! \brief Slot for the browse button; delegates to setRootFolder(). */
    void on_mediaFolderBrowseButton_clicked();

    /*! \brief Loads the folder path typed directly into the media folder line edit. */
    void on_mediaFolderLineEdit_returnPressed();

    /*! \brief Slot for the Save button; writes metadata for all dirty files. */
    void on_saveButton_clicked();

    /*! \brief Slot for File → Save All; writes metadata for all dirty files. */
    void on_actionSave_All_triggered();

    /*! \brief Slot for File → Save Visible; writes metadata only for files passing the current filter. */
    void on_actionSave_Visible_triggered();

    /*! \brief Removes a tag from all filtered files when an unassign is requested.
     *
     * \param tag The Tag to unassign from the filtered files.
     */
    void on_fileListTagAssignmentContainer_tagDeleteRequested(Tag* tag);

    /*! \brief Removes a tag from the active filter set when a filter-remove is requested.
     *
     * \param tag The Tag to remove from the filter.
     */
    void on_navFilterContainer_tagDeleteRequested(Tag* tag);

    /*! \brief Deletes a tag from the library when a delete is requested from the library panel.
     *
     * If no files carry the tag it is removed silently; otherwise the user is asked to confirm.
     * \param tag The Tag whose deletion was requested.
     */
    void onNavLibraryContainerTagDeleteRequested(Tag* tag);

    /*! \brief Updates the filename filter as the user types in the filter line edit.
     *
     * \param arg1 The current text of the filename filter line edit.
     */
    void on_fileNameFilterLineEdit_textChanged(const QString &arg1);

    /*! \brief Updates the folder filter as the user types in the folder filter line edit.
     *
     * \param arg1 The current text of the folder filter line edit.
     */
    void on_folderFilterLineEdit_textChanged(const QString &arg1);

    /*! \brief Advances the icon-generation progress bar when a thumbnail is ready. */
    void onIconUpdated();

    /*! \brief Advances the save progress bar when a metadata file is written. */
    void onMetadataSaved();

    /*! \brief Launches (or cancels) a multi-phase face recognition sweep across all loaded image files. */
    void on_actionFind_Faces_triggered();

    /*! \brief Opens the Face Recognition Settings dialog and applies any changes. */
    void on_actionFace_Recognition_Settings_triggered();

    /*! \brief Removes all auto-detected face tags from every file and from the tag library after user confirmation. */
    void on_actionRemove_Auto_Detected_Faces_triggered();

    /*! \brief Shows or hides tag region overlays in the preview when the checkbox is toggled.
     *
     * \param state The new checkbox state (Qt::Checked or Qt::Unchecked).
     */
    void on_showTaggedRegionsCheckbox_stateChanged(int state);

    /*! \brief Handles a tag drop on the preview; stores the region and refreshes overlays.
     *  \param family         Tag family name.
     *  \param tagName        Tag name.
     *  \param normalizedRect Normalized (0-1) bounding rect for the region.
     */
    void onTagDroppedOnPreview(const QString &family,
                               const QString &tagName,
                               const QRectF  &normalizedRect);

    /*! \brief Handles a tag drop onto an existing tag region; replaces the old tag with the dropped one.
     *
     * The region rect is preserved; only the tag assignment changes.
     *
     *  \param family       Tag family name of the dropped tag.
     *  \param tagName      Tag name of the dropped tag.
     *  \param existingRect Normalized rect of the region being replaced.
     */
    void onTagDroppedOnExistingRegion(const QString &family,
                                      const QString &tagName,
                                      const QRectF  &existingRect);

    /*! \brief Persists a resized tag region back to the TaggedFile.
     *  \param oldNorm Normalized rect before the resize (used to identify the tag).
     *  \param newNorm Normalized rect after the resize.
     */
    void onTagRectResized(const QRectF &oldNorm, const QRectF &newNorm);

    /*! \brief Removes the tag region at \p normalizedRect from the selected file.
     *
     * Finds the tag whose stored rect matches \p normalizedRect, removes it from
     * the TaggedFile, rebuilds the preview overlays, and refreshes the assignment area.
     *
     * \param normalizedRect Normalized (0–1) rect identifying the tag region to delete.
     */
    void onTagRectDeleteRequested(const QRectF &normalizedRect);

    /*! \brief Launches a "Find this person" search for the tag region at \p normalizedRect.
     *
     * Identifies the tag whose stored rect matches \p normalizedRect, shows a scope
     * selection dialog, then runs a background sweep that tags any matching faces
     * across the chosen files without creating auto-detected-face tags for non-matches.
     *
     * \param normalizedRect Normalized (0–1) rect identifying the tag region to use as query.
     */
    void onTagRectFindPersonRequested(const QRectF &normalizedRect);

    /*! \brief Slot — forwards creation-date filter changes to the core.
     *
     * A date equal to the widget's minimum is treated as "no filter" (null QDate).
     * \param date The newly selected date.
     */
    void on_dateEdit_dateChanged(const QDate &date);

    /*! \brief Rebuilds the tag-region overlays in the preview when a tag is renamed.
     *
     * Called whenever any TagContainer emits tagNameChanged().  Re-reads the
     * selected file's tag rects (which now carry the updated name) and pushes
     * them to the preview container so every overlay label reflects the new name.
     *
     * \param tag The Tag whose name was changed.
     */
    void onTagNameChanged(Tag* tag);

    /*! \brief Refreshes all tag-related containers after a merge changes the library. */
    void onTagLibraryChanged();

    /*! \brief Refreshes per-file preview UI elements not covered by onTagLibraryChanged().
     *
     * Called after snapshot restore (undo/redo) to update the preview star rating
     * for the currently selected file.
     */
    void onSnapshotRestored();

    /*! \brief Slot for Autos → Grab Video Frames; begins a frame-grab batch for all video files. */
    void on_actionGrab_Video_Frame_triggered();

    /*! \brief Receives a successfully captured frame and updates the in-memory icon for the file.
     *
     * \param path  Absolute path to the video file.
     * \param frame Captured frame (scaled to at most 400×400 px).
     * \param meta  Container metadata read from the video.
     */
    void onVideoFrameGrabbed(const QString &path, const QImage &frame,
                             const QMap<QString, QString> &meta);

    /*! \brief Logs a failed frame-capture attempt.
     *
     * \param path   Absolute path to the video file.
     * \param reason Human-readable description of the failure.
     * \param meta   Container metadata (may be partially populated).
     */
    void onVideoFrameFailed(const QString &path, const QString &reason,
                            const QMap<QString, QString> &meta);

    /*! \brief Advances the video-grab progress bar as each file completes.
     *
     * \param done  Number of files processed so far.
     * \param total Total number of files in the batch.
     */
    void onVideoGrabProgress(int done, int total);

    /*! \brief Shows the grab summary in the status bar and cleans up after the batch finishes.
     *
     * \param success Number of files from which a frame was captured.
     * \param fail    Number of files for which capture failed.
     */
    void onVideoGrabFinished(int success, int fail);

    /*! \brief Tags every file with its capture/creation year in the "Year" tag family. */
    void on_actionAuto_Tag_Year_triggered();

    /*! \brief Tags every file with its capture/creation month name in the "Month" tag family. */
    void on_actionAuto_Tag_Month_triggered();

    /*! \brief Isolates the currently selected files so only they pass the filter. */
    void on_actionIsolateSelection_triggered();

    /*! \brief Finds images similar to the current selection by perceptual hash and isolates them. */
    void on_actionFind_Similar_In_Selection_triggered();

    /*! \brief Finds all near-duplicate image groups by perceptual hash and isolates them. */
    void on_actionFind_Similar_Images_triggered();

    /*! \brief Clears the isolation set and restores the full unfiltered view. */
    void on_actionClearIsolation_triggered();

    /*! \brief Isolates all files under the folder of the first selected file. */
    void on_actionIsolateFolderSelection_triggered();

    /*! \brief Clears the folder isolation and restores the full unfiltered view. */
    void on_actionClearFolderIsolation_triggered();

    /*! \brief Resets all filter controls to their default empty state. */
    void on_actionClearAllFilters_triggered();

    /*! \brief Isolates files that could not be opened during icon generation. */
    void on_actionUnreadableFiles_triggered();

    /*! \brief Drills into the folder of the selected file, reloading with it as the new root. */
    void on_actionDrillToFolder_triggered();

    /*! \brief Navigates up one folder level, stopping at the drill ceiling. */
    void on_actionDrillUp_triggered();

    /*! \brief Slot for Help → Documentation; opens the online documentation in the default browser. */
    void on_actionDocumentation_triggered();

    /*! \brief Slot for Help → About Compendia; opens the About dialog. */
    void on_actionAbout_triggered();

    /*! \brief Slot for File → Export; copies all visible files to a user-chosen folder. */
    void on_actionExport_triggered();

protected:
    /*! \brief Overrides the Qt base-class resize handler to freshen the preview on resize.
     *
     * \param event The resize event containing the new window size.
     */
    void resizeEvent(QResizeEvent *event) override;

    /*! \brief Intercepts the window close event to prompt for unsaved changes.
     *
     * \param event The close event to accept or ignore.
     */
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
