#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QFuture>
#include <QItemSelection>
#include <QResizeEvent>
#include <QGraphicsView>
#include <QImageReader>
#include <QMessageBox>
#include "luminismcore.h"
#include "multiprogressbar.h"
#include "facerecognizer.h"
#include "facerecognitionsettingsdialog.h"
#include "framegrabber.h"
#include "constants.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/*! \brief The application's main window, wiring UI widgets to LuminismCore operations.
 *
 * MainWindow owns the generated Ui::MainWindow and a LuminismCore instance.
 * It does not own any file or tag data directly; all data lives in core.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    MultiProgressBar* progress_;
    QWidget* welcome_widget_; ///< Shown in the file-list area before any folder is loaded.
    FaceRecognizer* face_recognizer_ = nullptr;
    double faceMatchThreshold_ = Luminism::FaceMatchThreshold;
    QFuture<void> warmupFuture_; ///< Handle to the current background embedding warmup task.
    FrameGrabber* frameGrabber_ = nullptr; ///< Active frame-grab batch, or nullptr when idle.

public:
    /*! \brief Constructs the main window, sets up layouts, status bar, and default pane sizes.
     *
     * \param parent Optional Qt parent widget.
     */
    MainWindow(QWidget *parent = nullptr);

    /*! \brief Destroys the main window and releases owned UI and core resources. */
    ~MainWindow();

    /*! \brief The central application controller; provides access to data and business logic. */
    LuminismCore *core;

    /*! \brief Opens a folder-picker dialog and loads the selected folder into core. */
    void setRootFolder();

    /*! \brief Updates the file-count label to show visible vs. total file counts. */
    void updateFileCountLabel();

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
    /*! \brief Checks whether a luminismcache folder exists in \p folder and, if not,
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

    /*! \brief Triggers background known-face embedding warmup for \p tf if models are loaded.
     *
     * Collects all user-labeled tag regions from \p tf and passes them to
     * FaceRecognizer::warmupKnownFaceEmbeddings() via QtConcurrent::run.
     * Does nothing if face_recognizer_ is null or models are not yet loaded.
     *
     * \param tf The file being deselected. May be nullptr.
     */
    void scheduleEmbeddingWarmup(TaggedFile* tf);

    /*! \brief Validates \p folder, confirms cache creation if needed, then performs
     *         a full load: clears existing state, loads files into core, and refreshes
     *         all UI areas. Both the Browse and Return-key paths delegate here.
     *
     * \param folder Absolute path to the media folder to load.
     */
    void loadFolder(const QString &folder);

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

    /*! \brief Runs face detection on every selected image and stores face regions as tagged rectangles. */
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

    /*! \brief Switches the tag filter between ALL-tags (AND) and ANY-tag (OR) mode.
     *
     * \param checked True when the "ALL tags" radio button is selected.
     */
    void on_tagFilterAllRadio_toggled(bool checked);

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

    /*! \brief Clears the isolation set and restores the full unfiltered view. */
    void on_actionClearIsolation_triggered();

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
