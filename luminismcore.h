#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include <QDir>
#include <QDebug>
#include <QStandardItemModel>
#include <QMutex>
#include <QTimer>
#include <QtConcurrent>
#include <QMap>
#include <QList>
#include <QVector>
#include <QImage>

#include "tagset.h"
#include "filterproxymodel.h"
#include "icongenerator.h"
#include "taggedfile.h"

/*! \brief Central controller that owns all application data and business logic.
 *
 * LuminismCore manages the file list model, the tag library, and coordinates
 * asynchronous thumbnail generation with a timer-driven flush mechanism.
 * MainWindow holds a single instance of this class and delegates all
 * data operations to it.
 */
class LuminismCore : public QObject
{
    Q_OBJECT
private:

    QString root_directory_;

    QSet<TagFamily*>* tag_families_;
    QSet<Tag*>* tags_;
    QStandardItemModel *tagged_files_;
    FilterProxyModel *tagged_files_proxy_;

    QMutex resultsMutex_;
    QVector<std::tuple<QString, QString, QMap<QString, QString>, QVector<QImage>, quint64>> results_; // fileName, path, EXIF dict, images, pHash
    QTimer uiFlushTimer_;

    QPixmap default_icon_ = QPixmap(":/resources/NoImagePreviewIcon.png");

    IconGenerator *iconGenerator_ = nullptr;  ///< Active IconGenerator, or nullptr.

    /*! \brief Moves up to a fixed number of pending icon results from the background queue into the model.
     */
    void flushIconGeneratorQueue();

    /*! \brief Clears the dirty flag on every file, tag, and tag family. */
    void clearAllDirtyFlags();

    /*! \brief Applies a generated thumbnail, EXIF data, and pHash to the matching model item.
     *
     * \param fileName             The file name used to locate the model item.
     * \param absoluteFilePathName Full path used to disambiguate duplicate file names.
     * \param exifMap              EXIF key-value data to store on the TaggedFile.
     * \param images               Scaled thumbnail images to use as the item icon.
     * \param pHash                Perceptual hash (0 for videos / not computed).
     */
    void applyBackfillMetadataToModel(const QString &fileName,
                                      const QString &absoluteFilePathName,
                                      const QMap<QString, QString> exifMap,
                                      const QVector<QImage> &images,
                                      quint64 pHash);

public:

    /*! \brief Constructs LuminismCore and initialises empty model and tag-library containers.
     *
     * \param parent Optional Qt parent object.
     */
    explicit LuminismCore(QObject *parent = nullptr);

    /*! \brief Sets the root directory path and immediately loads it.
     *
     * \param path Absolute path to the media folder to load.
     */
    void setRootDirectory(QString path);

    /*! \brief Stops in-flight icon generation and discards any queued results.
     *
     * Does not affect in-progress metadata saves.
     */
    void cancelIconGeneration();

    /*! \brief Clears all existing data and reloads files from the current root directory. */
    void loadRootDirectory();

    /*! \brief Aggregate statistics about the currently loaded folder. */
    struct FolderStats {
        int imageCount  = 0; ///< Number of image files (excluding cache).
        int videoCount  = 0; ///< Number of video files (excluding cache).
        int folderCount = 0; ///< Number of distinct directories (excluding cache).
        qint64 totalBytes = 0; ///< Sum of file sizes in bytes (excluding cache).
    };

    /*! \brief Returns summary statistics for all files in the source model.
     *
     * Files inside any \c .luminism_cache sub-directory are excluded.
     * \return A FolderStats value with counts and total size.
     */
    FolderStats getFolderStats() const;

    /*! \brief Returns true if the model currently contains at least one file.
     *
     * \return True when the model row count is greater than zero.
     */
    bool containsFiles();

    /*! \brief Adds a file to the model with no tags.
     *
     * \param fileInfo File-system info for the file to add.
     */
    void addFile(QFileInfo fileInfo);

    /*! \brief Adds a file to the model, parsing tags and EXIF from a JSON object.
     *
     * \param fileInfo The file-system info for the file to add.
     * \param tagsJson A JSON object containing tag and optional EXIF data.
     */
    void addFile(QFileInfo fileInfo, QJsonObject tagsJson);

    /*! \brief Adds a file to the model with an explicit tag list and optional initial EXIF map.
     *
     * \param fileInfo    File-system info for the file to add.
     * \param tags        List of TagSet values describing tags to apply.
     * \param initialExif Optional pre-loaded EXIF key-value map.
     * \param pHash       Optional pre-loaded perceptual hash (0 if not available).
     * \param rating      Optional pre-loaded rating (1–5), or std::nullopt if not set.
     */
    void addFile(QFileInfo fileInfo, QList<TagSet> tags, QMap<QString, QString> initialExif = {}, quint64 pHash = 0, std::optional<int> rating = std::nullopt);

    /*! \brief Launches background thumbnail and EXIF generation for all files in the model. */
    void backfillMetadata();

    /*! \brief Writes JSON sidecar files for every file with unsaved changes. */
    void writeFileMetadata();

    /*! \brief Writes JSON sidecar files for every visible (proxy-filtered) file with unsaved changes. */
    void writeVisibleFileMetadata();

    /*! \brief Returns true if any file in the model has unsaved changes.
     *
     * \return True when at least one TaggedFile has a dirty flag set.
     */
    bool hasUnsavedChanges();

    /*! \brief Returns a pointer to the complete tag library.
     *
     * \return Pointer to the set of all known Tag objects.
     */
    QSet<Tag*>* getLibraryTags();

    /*! \brief Returns a new set of tags that are assigned to at least one file.
     *
     * \return Heap-allocated set of assigned Tag pointers (caller takes ownership).
     */
    QSet<Tag*>* getAssignedTags();

    /*! \brief Returns a new set of tags assigned to any currently visible (filtered) file.
     *
     * \return Heap-allocated set of Tag pointers (caller takes ownership).
     */
    QSet<Tag*>* getAssignedTags_FilteredFiles();

    /*! \brief Returns a pointer to the set of tags currently active as filters.
     *
     * \return Pointer to the proxy model's internal filter tag set.
     */
    QSet<Tag*>* getFilterTags();

    /*! \brief Retrieve a tag from the library by name.
     *
     * \param tagFamilyName The name of the tag family to search within.
     * \param tagName       The tag name to find.
     * \return Pointer to the matching Tag, or nullptr if not found.
     */
    Tag* getTag(QString tagFamilyName, QString tagName);

    /*! \brief Retrieve a tag family from the library by name.
     *
     * \param tagFamilyName The family name to find.
     * \return Pointer to the matching TagFamily, or nullptr if not found.
     */
    TagFamily* getTagFamily(QString tagFamilyName);

    /*! \brief Creates or retrieves a tag in the library by family name and tag name.
     *
     * \param tagFamily The name of the family the tag belongs to.
     * \param tagName   The name of the tag to create or find.
     * \return Pointer to the new or existing Tag.
     */
    Tag* addLibraryTag(QString tagFamily, QString tagName);

    /*! \brief Adds an existing Tag pointer to the library if it is not already present.
     *
     * \param tag The Tag pointer to insert.
     */
    void addLibraryTag(Tag* tag);

    /*! \brief Creates or retrieves a tag family in the library by name.
     *
     * \param tagFamilyName The family name to create or find.
     * \return Pointer to the new or existing TagFamily.
     */
    TagFamily* addLibraryTagFamily(QString tagFamilyName);

    /*! \brief Adds an existing TagFamily pointer to the library if not already present.
     *
     * \param tagFamily The TagFamily pointer to insert.
     */
    void addLibraryTagFamily(TagFamily* tagFamily);

    /*! \brief Applies a tag to all files currently visible in the filtered file list.
     *
     * \param tag The Tag to apply.
     */
    void applyTag(Tag* tag);

    /*! \brief Applies the tag identified by a TagSet to a specific file.
     *
     * \param file   The TaggedFile to tag.
     * \param tagSet The TagSet identifying the tag to apply.
     */
    void applyTag(TaggedFile* file, TagSet tagSet);

    /*! \brief Removes a tag from a specific file.
     *
     * \param file The TaggedFile to remove the tag from.
     * \param tag  The Tag to remove.
     */
    void unapplyTag(TaggedFile* file, Tag* tag);

    /*! \brief Removes a tag from all files currently visible in the filtered file list.
     *
     * \param tag The Tag to remove.
     */
    void unapplyTag(Tag* tag);

    /*! \brief Returns the number of currently visible files that have the given tag assigned.
     *
     * \param tag The Tag to count.
     * \return Count of visible files carrying the tag.
     */
    int countVisibleFilesWithTag(Tag* tag);

    /*! \brief Returns the number of all files (ignoring the proxy filter) that have the given tag.
     *
     * \param tag The Tag to count.
     * \return Count of all files carrying the tag.
     */
    int countAllFilesWithTag(Tag* tag);

    /*! \brief Removes a tag from all files, the filter, and the library, then deletes it.
     *
     * Emits tagLibraryChanged() when done.
     * \param tag The Tag to delete.
     */
    void deleteTagFromLibrary(Tag* tag);

    /*! \brief Sets the filename substring filter on the proxy model.
     *
     * \param filterText The substring to match against file names.
     */
    void setFileNameFilter(QString filterText);

    /*! \brief Sets the folder path substring filter on the proxy model.
     *
     * \param filterText The substring to match against folder paths.
     */
    void setFolderFilter(QString filterText);

    /*! \brief Sets the creation-date filter on the proxy model.
     *
     * \param date The date to filter by; an invalid QDate clears the filter.
     */
    void setCreationDateFilter(QDate date);

    /*! \brief Sets the rating filter on the proxy model.
     *
     * Only files whose rating satisfies the current mode vs. \p rating will pass.
     * Pass std::nullopt to clear the rating filter.
     *
     * \param rating The rating to filter for [1,5], or std::nullopt to disable.
     */
    void setRatingFilter(std::optional<int> rating);

    /*! \brief Sets the comparison mode used when evaluating the rating filter.
     *
     * \param mode LessOrEqual, Exactly, or GreaterOrEqual.
     */
    void setRatingFilterMode(FilterProxyModel::RatingFilterMode mode);

    /*! \brief Returns a chronologically sorted list of unique effective dates across all loaded files.
     *
     * Each file's effective date is its EXIF capture date when present, falling back to the
     * filesystem creation date.  Used to populate the date-filter autocomplete.
     */
    QList<QDate> getFileDates() const;

    /*! \brief Returns a sorted list of unique folder paths across all loaded files.
     *
     * Used to populate the folder-filter autocomplete.
     */
    QList<QString> getFileFolders() const;

    /*! \brief Adds a tag to the active tag filter set.
     *
     * \param tag The Tag to add to the filter.
     */
    void addTagFilter(Tag* tag);

    /*! \brief Removes a tag from the active tag filter set.
     *
     * \param tag The Tag to remove from the filter.
     */
    void removeTagFilter(Tag* tag);

    /*! \brief Restricts the visible set to the given files; other filters still apply within the set.
     *
     * \param files The set of TaggedFile pointers to isolate.
     */
    void setIsolationSet(const QSet<TaggedFile*> &files);

    /*! \brief Clears the isolation set so all files are eligible again.
     */
    void clearIsolationSet();

    /*! \brief Returns true when an isolation set is active.
     *
     * \return True if the proxy model has a non-empty isolation set.
     */
    bool isIsolated() const;

    /*! \brief Returns the number of files in the current isolation set.
     *
     * \return Size of the isolation set, or 0 when not isolated.
     */
    int isolationSetSize() const;

    /*! \brief Returns the underlying QStandardItemModel containing all file items.
     *
     * \return Pointer to the source item model.
     */
    QStandardItemModel* getItemModel();

    /*! \brief Returns the FilterProxyModel used by the file list view.
     *
     * \return Pointer to the proxy model.
     */
    FilterProxyModel* getItemModelProxy();

    /*! \brief Parses a JSON object of tag-family to tag-name arrays into a TagSet list.
     *
     * \param tagsJson     A JSON object mapping family name strings to arrays of tag name strings.
     * \param tagRectsJson Optional JSON object mapping family → tag → [x,y,w,h] for bounding rects.
     * \return A list of TagSet values parsed from the JSON.
     */
    QList<TagSet> parseTagJson(QJsonObject tagsJson, QJsonObject tagRectsJson = QJsonObject());

    /*! \brief Updates the in-memory icon for the file at \p absoluteFilePath using the provided images.
     *
     * Iterates the source model, finds the item whose TaggedFile path matches
     * \p absoluteFilePath, replaces its decoration with a multi-size QIcon built
     * from \p images, and emits iconUpdated().  The on-disk cache is not written;
     * a folder reload will re-trigger normal icon generation.
     *
     * \param absoluteFilePath Absolute path to the video file whose icon to replace.
     * \param images           Scaled thumbnail images (one per IconGenerator::kIconSizes entry).
     */
    void updateFileIcons(const QString &absoluteFilePath, const QVector<QImage> &images);

    /*! \brief Applies video container metadata to the TaggedFile for \p absoluteFilePath.
     *
     * Sets the file's exifMap and, when a \c Date key is present, its imageCaptureDateTime.
     * Called after a manual "Grab Video Frames" so the preview panel shows metadata.
     *
     * \param absoluteFilePath Absolute path to the video file.
     * \param meta             Container metadata map produced by FrameGrabber.
     */
    void applyVideoMetadata(const QString &absoluteFilePath,
                            const QMap<QString, QString> &meta);

    /*! \brief Groups all files whose pHash values are within \a threshold Hamming distance of each other.
     *
     * Uses a Union-Find algorithm over all files with a valid pHash.  Only groups of
     * two or more files are returned.
     *
     * \param threshold Maximum Hamming distance to consider two images near-duplicates.
     * \return List of groups; each group contains two or more TaggedFile pointers.
     */
    QList<QList<TaggedFile*>> findSimilarImages(int threshold);

    /*! \brief Removes all auto-detected face tags from every file, the active
     *  filter, and the tag library. Cleans up now-empty tag families and emits
     *  tagLibraryChanged() once when done.
     *
     * \return Number of tags removed, or 0 if there were none.
     */
    int removeAutoDetectedFaceTags();

    /*! \brief Merges \a from into \a into across all files, then removes and schedules \a from for deletion.
     *  Emits tagLibraryChanged() when done. */
    void mergeTag(Tag* from, Tag* into);

    /*! \brief Merges all tags of family \a from into family \a into, handling per-tag collisions
     *  recursively, then removes and schedules \a from for deletion. Emits tagLibraryChanged(). */
    void mergeTagFamily(TagFamily* from, TagFamily* into);

public slots:

    /*! \brief Starts the UI flush timer if it is not already running.
     *
     * Invoked from background threads via a queued connection so the timer
     * runs on the GUI thread.
     */
    void ensureUiFlushTimerRunning();

private slots:
    /*! \brief Receives a completed file result from IconGenerator and pushes it to the flush queue.
     *
     * \param absolutePath Absolute path to the source file.
     * \param exifMap      EXIF data (empty for videos).
     * \param images       Scaled thumbnail images.
     * \param pHash        Perceptual hash (0 for videos).
     */
    void onIconReady(const QString &absolutePath,
                     const QMap<QString, QString> &exifMap,
                     const QVector<QImage> &images,
                     quint64 pHash);

    /*! \brief Called when IconGenerator finishes all files in the batch. */
    void onIconBatchFinished();

signals:
    /*! \brief Emitted each time a thumbnail icon is applied to the model. */
    void iconUpdated();
    /*! \brief Emitted each time a sidecar metadata file is written successfully. */
    void metadataSaved();
    /*! \brief Emitted after a tag or tag-family merge alters the library. */
    void tagLibraryChanged();
    /*! \brief Emitted once when the current icon generation batch is fully complete. */
    void batchFinished();
};

#endif // LUMINISMCORE_H
