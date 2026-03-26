#include "compendiacore.h"
#include "constants.h"
#include "perceptualhasher.h"
#include "folderscanner.h"
#include "exifparser.h"
#include "version.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDebug>
#include <QIcon>
#include <QSet>
#include <QRunnable>
#include <QThreadPool>

/*! \brief Constructs CompendiaCore and initialises empty model and tag-library containers.
 *
 * \param parent Optional Qt parent object.
 */
CompendiaCore::CompendiaCore(QObject *parent)
    : QObject{parent}{

    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();

    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);
    tagged_files_proxy_->setSortCaseSensitivity(Qt::CaseInsensitive);
    tagged_files_proxy_->sort(0);

    fileWatcher_ = new QFileSystemWatcher(this);
    connect(fileWatcher_, &QFileSystemWatcher::directoryChanged,
            this, &CompendiaCore::onWatchedDirectoryChanged);
    connect(fileWatcher_, &QFileSystemWatcher::fileChanged,
            this, &CompendiaCore::onWatchedFileChanged);

    watcherDebounceTimer_.setSingleShot(true);
    watcherDebounceTimer_.setInterval(300);
    connect(&watcherDebounceTimer_, &QTimer::timeout,
            this, &CompendiaCore::processWatcherChanges);

    wantedUpdateTimer_.setSingleShot(true);
    wantedUpdateTimer_.setInterval(80);
    connect(&wantedUpdateTimer_, &QTimer::timeout, this, &CompendiaCore::updateWantedPaths);

    connect(this, &CompendiaCore::tagLibraryChanged, this, &CompendiaCore::writeTagLibraryFile);
}

void CompendiaCore::setListView(QListView *view)
{
    listView_ = view;
}

void CompendiaCore::scheduleWantedUpdate()
{
    wantedUpdateTimer_.start();   // restarts (debounces) on repeated calls
}

void CompendiaCore::updateWantedPaths()
{
    if (!listView_ || !tagged_files_proxy_)
        return;

    const QRect vp       = listView_->viewport()->rect();
    const int   rowCount = tagged_files_proxy_->rowCount();
    if (rowCount == 0)
        return;

    QModelIndex topIdx    = listView_->indexAt(vp.topLeft());
    QModelIndex bottomIdx = listView_->indexAt(vp.bottomRight());

    int firstRow = topIdx.isValid()    ? topIdx.row()    : 0;
    int lastRow  = bottomIdx.isValid() ? bottomIdx.row() : rowCount - 1;

    const int kBuffer = 20;
    firstRow = qMax(0,            firstRow - kBuffer);
    lastRow  = qMin(rowCount - 1, lastRow  + kBuffer);

    QSet<QString> wanted;
    wanted.reserve(lastRow - firstRow + 1);
    for (int row = firstRow; row <= lastRow; ++row) {
        QModelIndex proxyIdx = tagged_files_proxy_->index(row, 0);
        QModelIndex srcIdx   = tagged_files_proxy_->mapToSource(proxyIdx);
        QStandardItem *item  = tagged_files_->itemFromIndex(srcIdx);
        if (!item) continue;
        TaggedFile *tf = item->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf)
            wanted.insert(tf->filePath + "/" + tf->fileName);
    }

    {
        QWriteLocker lock(&wantedMutex_);
        wantedIconPaths_ = std::move(wanted);
    }
}

/*! \brief Moves up to a fixed number of pending icon results from the background queue into the model. */
void CompendiaCore::flushIconGeneratorQueue(){
    const int maxPerTick = 8;
    QVector<std::tuple<QString, QString, QMap<QString, QString>, QVector<QImage>, quint64>> batch;
    {
        QMutexLocker lock(&resultsMutex_);
        int take = qMin(maxPerTick, results_.size());
        for (int i = 0; i < take; ++i) batch.append(std::move(results_.takeFirst()));
        if (results_.isEmpty()) uiFlushTimer_.stop();
    }

    for (auto &t : batch) {
        const QString &fileName = std::get<0>(t);
        const QString &path = std::get<1>(t);
        QMap<QString, QString> exifMap = std::get<2>(t);
        QVector<QImage> images = std::get<3>(t);
        quint64 pHash = std::get<4>(t);

        applyBackfillMetadataToModel(fileName, path, exifMap, images, pHash);
        emit iconUpdated();
    }
}

/*! \brief Applies icon images, EXIF data, and pHash directly to a model item.
 *
 * The icon is stored in the LRU pool (iconPool_) rather than on the item, so
 * that total icon memory remains bounded regardless of folder size.
 */
void CompendiaCore::applyIconDataToItem(QStandardItem *item,
                                       const QVector<QImage> &images,
                                       const QMap<QString, QString> &exifMap,
                                       quint64 pHash)
{
    TaggedFile *tf = item->data(Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    QIcon icon;
    for (const QImage &image : images) {
        int sz = qMax(image.width(), image.height());
        QPixmap square(sz, sz);
        square.fill(Qt::transparent);
        QPainter painter(&square);
        int x = (sz - image.width()) / 2;
        int y = (sz - image.height()) / 2;
        painter.drawImage(x, y, image);
        painter.end();
        icon.addPixmap(square);
    }

    // Store in the LRU pool instead of on the item, keeping memory bounded.
    const QString path = tf->filePath + "/" + tf->fileName;
    pendingIconLoads_.remove(path);
    iconPool_.insert(path, new QIcon(icon), 1);

    QString captureDateString = exifMap["DateTime"];
    QDateTime captureDateTime = QDateTime::fromString(captureDateString, "yyyy:MM:dd HH:mm:ss");
    if (!captureDateTime.isValid() && exifMap.contains("Date"))
        captureDateTime = QDateTime::fromString(exifMap["Date"], Qt::ISODate);

    tf->imageCaptureDateTime = captureDateTime;
    if (!exifMap.isEmpty())
        tf->setExifMap(exifMap);
    if (pHash != 0)
        tf->setPHash(pHash);

    // Notify the view to repaint this item so the delegate picks up the new icon.
    QModelIndex idx = tagged_files_->indexFromItem(item);
    if (idx.isValid())
        emit tagged_files_->dataChanged(idx, idx, {Qt::DecorationRole});
}

/*! \brief Applies generated thumbnails and EXIF data to the matching model item.
 *
 * \param fileName             The file name used to locate the model item.
 * \param absoluteFilePathName Full path used to disambiguate duplicate file names.
 * \param exifMap              EXIF key-value data to store on the TaggedFile.
 * \param images               Scaled thumbnail images (one per kIconSizes entry) used to
 *                             build a multi-size QIcon so Qt always downscales cleanly.
 */
void CompendiaCore::applyBackfillMetadataToModel(const QString &fileName,
                                                const QString &absoluteFilePathName,
                                                const QMap<QString, QString> exifMap,
                                                const QVector<QImage> &images,
                                                quint64 pHash)
{
    // There could be files in different folders having the same name, but to make things quick
    // we find all files with a matching name in the model, and then zero in on the specific one
    // using the full path and name. Generally expect the names will be unique most of the time.

    auto matches = tagged_files_->findItems(fileName);
    if (matches.isEmpty()) {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
        return;
    }

    // Handle duplicate file names by comparing on the full path
    QStandardItem* item = nullptr;
    TaggedFile* tf = nullptr;
    for (int i = 0; i < matches.count(); ++i){
        QStandardItem* currentItem = matches[i];
        QVariant var = currentItem->data(Qt::UserRole + 1);
        tf = var.value<TaggedFile*>();

        if((tf->filePath + "/" + tf->fileName) == absoluteFilePathName)
            item = currentItem;
    }

    if (item != nullptr)
        applyIconDataToItem(item, images, exifMap, pHash);
    else
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
}

/*! \brief Updates the in-memory icon for the file at \p absoluteFilePath using the provided images.
 *
 * \param absoluteFilePath Absolute path to the video file whose icon to replace.
 * \param images           Scaled thumbnail images (one per IconGenerator::kIconSizes entry).
 */
void CompendiaCore::updateFileIcons(const QString &absoluteFilePath, const QVector<QImage> &images)
{
    QFileInfo fi(absoluteFilePath);
    const QString fileName = fi.fileName();

    auto matches = tagged_files_->findItems(fileName);
    if (matches.isEmpty()) {
        qDebug() << "updateFileIcons: could not locate" << absoluteFilePath;
        return;
    }

    for (QStandardItem *candidate : matches) {
        TaggedFile *tf = candidate->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf)
            continue;
        if ((tf->filePath + "/" + tf->fileName) != absoluteFilePath)
            continue;

        QIcon icon;
        for (const QImage &image : images) {
            int sz = qMax(image.width(), image.height());
            QPixmap square(sz, sz);
            square.fill(Qt::transparent);
            QPainter painter(&square);
            int x = (sz - image.width()) / 2;
            int y = (sz - image.height()) / 2;
            painter.drawImage(x, y, image);
            painter.end();
            icon.addPixmap(square);
        }
        iconPool_.insert(absoluteFilePath, new QIcon(icon), 1);
        QModelIndex idx = tagged_files_->indexFromItem(candidate);
        if (idx.isValid())
            emit tagged_files_->dataChanged(idx, idx, {Qt::DecorationRole});
        emit iconUpdated();
        return;
    }

    qDebug() << "updateFileIcons: could not match path for" << absoluteFilePath;
}

/*! \brief Returns the cached QIcon for \p absoluteFilePath from the LRU pool.
 *
 * If not in the pool, schedules an async disk read and returns a null QIcon.
 */
QIcon CompendiaCore::iconForPath(const QString &absoluteFilePath)
{
    if (QIcon *cached = iconPool_.object(absoluteFilePath))
        return *cached;

    scheduleIconLoad(absoluteFilePath);
    return QIcon();
}

/*! \brief Schedules a background disk read for the cached thumbnails of \p path. */
void CompendiaCore::scheduleIconLoad(const QString &path)
{
    if (pendingIconLoads_.contains(path))
        return;

    // Only schedule if the view currently wants this icon.
    // wantedIconPaths_ is empty until setListView() is called (first loadFolder),
    // so skip the guard when listView_ is null to avoid blocking early loads.
    if (listView_ && !wantedIconPaths_.contains(path))
        return;

    pendingIconLoads_.insert(path);

    auto *runnable = QRunnable::create([this, path]() {
        // Cross-thread staleness check before any disk I/O.
        {
            QReadLocker lock(&wantedMutex_);
            if (!wantedIconPaths_.contains(path)) {
                // Path is no longer wanted. Remove from pending so it can be
                // re-queued when it scrolls back into view.
                QMetaObject::invokeMethod(this, [this, path]() {
                    pendingIconLoads_.remove(path);
                }, Qt::QueuedConnection);
                return;
            }
        }

        if (!IconGenerator::iconCacheValid(path))
            return;

        QVector<QImage> imgs = IconGenerator::loadIconsFromCache(path);
        if (imgs.size() != IconGenerator::kIconSizes.size())
            return;

        // Build the QIcon and update the pool on the main thread (QPixmap is GUI-only).
        QMetaObject::invokeMethod(this, [this, path, imgs]() {
            pendingIconLoads_.remove(path);

            QIcon icon;
            for (const QImage &image : imgs) {
                int sz = qMax(image.width(), image.height());
                QPixmap square(sz, sz);
                square.fill(Qt::transparent);
                QPainter painter(&square);
                painter.drawImage((sz - image.width()) / 2, (sz - image.height()) / 2, image);
                painter.end();
                icon.addPixmap(square);
            }
            iconPool_.insert(path, new QIcon(icon), 1);

            const QString fileName = QFileInfo(path).fileName();
            const auto matches = tagged_files_->findItems(fileName);
            for (QStandardItem *item : matches) {
                TaggedFile *tf = item->data(Qt::UserRole + 1).value<TaggedFile*>();
                if (tf && (tf->filePath + "/" + tf->fileName) == path) {
                    QModelIndex idx = tagged_files_->indexFromItem(item);
                    if (idx.isValid())
                        emit tagged_files_->dataChanged(idx, idx, {Qt::DecorationRole});
                    break;
                }
            }
        });
    });
    runnable->setAutoDelete(true);
    QThreadPool::globalInstance()->start(runnable);
}

/*! \brief Applies video container metadata to the TaggedFile for \p absoluteFilePath.
 *
 * \param absoluteFilePath Absolute path to the video file.
 * \param meta             Container metadata map produced by FrameGrabber.
 */
void CompendiaCore::applyVideoMetadata(const QString &absoluteFilePath,
                                      const QMap<QString, QString> &meta)
{
    QFileInfo fi(absoluteFilePath);
    const QString fileName = fi.fileName();

    auto matches = tagged_files_->findItems(fileName);
    for (QStandardItem *candidate : matches) {
        TaggedFile *tf = candidate->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf)
            continue;
        if ((tf->filePath + "/" + tf->fileName) != absoluteFilePath)
            continue;

        tf->setExifMap(meta);

        if (meta.contains("Date")) {
            QDateTime dt = QDateTime::fromString(meta["Date"], Qt::ISODate);
            if (dt.isValid())
                tf->imageCaptureDateTime = dt;
        }
        return;
    }

    qDebug() << "applyVideoMetadata: could not locate" << absoluteFilePath;
}

/*! \brief Sets the root directory path and immediately loads it.
 *
 * \param path Absolute path to the media folder to load.
 */
void CompendiaCore::setRootDirectory(QString path){
    root_directory_ = path;
    loadRootDirectory();
}

void CompendiaCore::cancelIconGeneration(){
    // Cancel any in-flight folder scan first.
    ++scanGeneration_;  // invalidates any pending onScanBatch / onScanFinished callbacks
    if (folderScanner_) {
        folderScanner_->cancel();
        folderScanner_ = nullptr;
    }
    if (scanThread_) {
        scanThread_->quit();
        scanThread_->wait();
        scanThread_ = nullptr;
    }

    if (iconGenerator_) {
        iconGenerator_->disconnect();
        iconGenerator_->deleteLater();
        iconGenerator_ = nullptr;
    }
    uiFlushTimer_.stop();
    QMutexLocker lock(&resultsMutex_);
    results_.clear();
}

/*! \brief Clears all existing data and starts an async scan of the current root directory. */
void CompendiaCore::loadRootDirectory(){

    // Clear any previously watched paths before loading a new folder.
    if (fileWatcher_) {
        if (!fileWatcher_->directories().isEmpty())
            fileWatcher_->removePaths(fileWatcher_->directories());
        if (!fileWatcher_->files().isEmpty())
            fileWatcher_->removePaths(fileWatcher_->files());
    }

    libraryFileInitialized_ = false;

    // Pointing to a new folder means we have to clear any data already loaded.
    // (cancelIconGeneration() has already been called by the time we get here.)
    delete tagged_files_proxy_;
    delete tagged_files_;
    delete tags_;
    delete tag_families_;

    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();
    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);
    tagged_files_proxy_->setSortCaseSensitivity(Qt::CaseInsensitive);
    uncachedPaths_.clear();
    iconPool_.clear();
    pendingIconLoads_.clear();
    {
        QWriteLocker lock(&wantedMutex_);
        wantedIconPaths_.clear();
    }

    // Start a background scan.  The generation counter is already up-to-date
    // because cancelIconGeneration() incremented it; capture the current value
    // in the lambdas so stale callbacks from a previously cancelled scan are ignored.
    int generation = scanGeneration_;

    scanThread_    = new QThread(this);
    folderScanner_ = new FolderScanner;
    folderScanner_->moveToThread(scanThread_);

    connect(scanThread_, &QThread::started,
            folderScanner_, [this]{ folderScanner_->scan(root_directory_); });

    connect(folderScanner_, &FolderScanner::batchReady,
            this, [this, generation](QList<ScanItem> batch) {
        if (scanGeneration_ == generation)
            onScanBatch(std::move(batch));
    });

    connect(folderScanner_, &FolderScanner::finished,
            this, [this, generation]() {
        if (scanGeneration_ == generation)
            onScanFinished();
    });

    connect(folderScanner_, &FolderScanner::finished, scanThread_, &QThread::quit);
    connect(scanThread_,    &QThread::finished, folderScanner_, &QObject::deleteLater);
    connect(scanThread_,    &QThread::finished, scanThread_,    &QObject::deleteLater);

    emit scanStarted();
    scanThread_->start();
}

/*! \brief Returns true if the model currently contains at least one file.
 *
 * \return True when the model row count is greater than zero.
 */
bool CompendiaCore::containsFiles(){
    return ( tagged_files_->rowCount() > 0 );
}

/*! \brief Returns summary statistics for all files in the source model.
 *
 * Files inside any .compendia_cache sub-directory are excluded.
 * \return A FolderStats value with image/video counts, folder count, and total size.
 */
CompendiaCore::FolderStats CompendiaCore::getFolderStats() const
{
    FolderStats stats;
    QSet<QString> folders;
    const int n = tagged_files_->rowCount();
    for (int i = 0; i < n; ++i) {
        const QVariant val = tagged_files_->item(i)->data(Qt::UserRole + 1);
        const TaggedFile *tf = val.value<TaggedFile*>();
        if (!tf) continue;
        if (tf->filePath.contains(QLatin1String(Compendia::CacheFolderName))) continue;
        if (tf->mediaType() == TaggedFile::Image) ++stats.imageCount;
        else ++stats.videoCount;
        stats.totalBytes += tf->fileSize();
        folders.insert(tf->filePath);
    }
    stats.folderCount = folders.size();
    return stats;
}

/*! \brief Adds a file to the model with no tags.
 *
 * \param fileInfo File-system info for the file to add.
 */
void CompendiaCore::addFile(QFileInfo fileInfo){
    QList<TagSet> tagSets;
    addFile(fileInfo, tagSets);
}

/*! \brief Adds a file to the model, parsing tags and EXIF from a JSON object.
 *
 * \param fileInfo The file-system info for the file to add.
 * \param tagsJson A JSON object containing tag and optional EXIF data.
 */
void CompendiaCore::addFile(QFileInfo fileInfo, QJsonObject tagsJson){
    QList<TagSet> tagSets;
    QMap<QString, QString> exifMap;
    quint64 pHash = 0;

    QJsonObject tagRectsJson = tagsJson["tag_rects"].toObject();
    tagSets = parseTagJson(tagsJson["tags"].toObject(), tagRectsJson);
    QJsonObject exifObj = tagsJson["exif"].toObject();
    for (auto it = exifObj.begin(); it != exifObj.end(); ++it) {
        exifMap.insert(it.key(), it.value().toString());
    }
    if (tagsJson.contains("pHash")) {
        bool ok = false;
        quint64 h = tagsJson["pHash"].toString().toULongLong(&ok, 16);
        if (ok) pHash = h;
    }

    std::optional<int> rating = std::nullopt;
    if (tagsJson.contains("rating")) {
        int r = tagsJson["rating"].toInt();
        if (r >= 1 && r <= 5)
            rating = r;
    }

    addFile(fileInfo, tagSets, exifMap, pHash, rating);
}

/*! \brief Adds a file to the model with an explicit tag list and optional initial EXIF map.
 *
 * \param fileInfo    File-system info for the file to add.
 * \param tags        List of TagSet values describing tags to apply.
 * \param initialExif Optional pre-loaded EXIF key-value map.
 */
void CompendiaCore::addFile(QFileInfo fileInfo, QList<TagSet> tags, QMap<QString, QString> initialExif, quint64 pHash, std::optional<int> rating){
    // Add to to the Tagged Object collection and collect links to its tags and tag families

    // For each item in the tag set add the family, if it doesn't already exist,
    // and add the tag including a link to the family, if the tag doesn't already exist

    QSet<Tag*>* newOrExistingTags = new QSet<Tag*>();

    for(const TagSet value : tags) {

        TagFamily *current_family = nullptr;

        for(TagFamily *fam : *tag_families_) {
            if (fam->getName() == value.tagFamilyName){
                // Found a match, use it
                current_family = fam;
            }
        }

        if( ! current_family ){
            current_family = new TagFamily(value.tagFamilyName, this);
            tag_families_->insert(current_family);
            connect(current_family, &TagFamily::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
        }

        Tag *current_tag = nullptr;

        for(Tag *tag : *tags_){
            if (tag->tagFamily->getName() == value.tagFamilyName && tag->getName() == value.tagName){
                current_tag = tag;
            }
        }

        if( ! current_tag ){
            current_tag = new Tag(current_family, value.tagName, this);
            tags_->insert(current_tag);
            connect(current_tag, &Tag::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
        }

        newOrExistingTags->insert(current_tag);
    }

    // Then add it
    TaggedFile *tf = new TaggedFile(fileInfo, newOrExistingTags, new QMap<QString, QString>, this);
    tf->initExifMap(initialExif);
    if (pHash != 0)
        tf->initPHash(pHash);
    if (rating.has_value())
        tf->initRating(rating);

    // Apply any bounding rectangles carried in the TagSets (without dirtying)
    for (const TagSet &ts : tags) {
        if (ts.rect.has_value()) {
            Tag* tag = getTag(ts.tagFamilyName, ts.tagName);
            if (tag) tf->initTagRect(tag, ts.rect.value());
        }
    }

    // Standard items have just an icon and text

    // Make an icon moved to run async
    QPixmap squarePixmap = default_icon_;
    QStandardItem *i = new QStandardItem(QIcon(squarePixmap), tf->fileName);
    // In order to store the custom object box it as a variant and store in item.setdata()
    i->setData(QVariant::fromValue(tf));
    tagged_files_->appendRow(i);
}

/*! \brief Launches background thumbnail and EXIF generation for all files in the model. */
void CompendiaCore::backfillMetadata(){

    // Fix pre-existing duplicate-connection bug by disconnecting first
    uiFlushTimer_.disconnect();
    connect(&uiFlushTimer_, &QTimer::timeout, this, &CompendiaCore::flushIconGeneratorQueue);
    uiFlushTimer_.setInterval(50);

    QStringList pathsToProcess;
    pathsToProcess.swap(uncachedPaths_);   // consume and clear in one step

    std::sort(pathsToProcess.begin(), pathsToProcess.end(), [](const QString &a, const QString &b) {
        return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
    });

    iconGenerator_ = new IconGenerator(this);
    connect(iconGenerator_, &IconGenerator::fileReady,
            this, &CompendiaCore::onIconReady);
    connect(iconGenerator_, &IconGenerator::batchFinished,
            this, &CompendiaCore::onIconBatchFinished);
    iconGenerator_->processFiles(pathsToProcess);
}

/*! \brief Receives a completed file result from IconGenerator and pushes it to the flush queue.
 *
 * \param absolutePath Absolute path to the source file.
 * \param exifMap      EXIF data (empty for videos).
 * \param images       Scaled thumbnail images.
 */
void CompendiaCore::onIconReady(const QString &absolutePath,
                               const QMap<QString, QString> &exifMap,
                               const QVector<QImage> &images,
                               quint64 pHash)
{
    QString fileName = QFileInfo(absolutePath).fileName();
    {
        QMutexLocker lock(&resultsMutex_);
        results_.emplace_back(fileName, absolutePath, exifMap, images, pHash);
    }
    ensureUiFlushTimerRunning();
}

/*! \brief Called when IconGenerator finishes all files in the batch. */
void CompendiaCore::onIconBatchFinished()
{
    // All fileReady signals have been emitted, so results_ is fully populated.
    // Flush any items that the 50 ms timer hasn't processed yet before announcing
    // completion — otherwise the last few icons would never reach the model.
    while (!results_.isEmpty())
        flushIconGeneratorQueue();
    uiFlushTimer_.stop();

    emit batchFinished();
    iconGenerator_->deleteLater();
    iconGenerator_ = nullptr;
}

/*! \brief Receives a batch of scanned files from the background FolderScanner. */
void CompendiaCore::onScanBatch(QList<ScanItem> batch)
{
    for (const ScanItem &item : batch) {
        if (item.hasJson)
            addFile(item.fileInfo, item.tagsJson);
        else
            addFile(item.fileInfo);

        if (item.hasCachedIcon) {
            // Valid cache on disk: the delegate will load the icon on demand.
            // Apply EXIF now so date-based sorting/filtering is correct immediately.
            QStandardItem *si = tagged_files_->item(tagged_files_->rowCount() - 1);
            if (si && !item.cachedExif.isEmpty()) {
                TaggedFile *tf = si->data(Qt::UserRole + 1).value<TaggedFile*>();
                if (tf) {
                    tf->initExifMap(item.cachedExif);
                    QDateTime dt = QDateTime::fromString(item.cachedExif["DateTime"],
                                                         "yyyy:MM:dd HH:mm:ss");
                    if (dt.isValid())
                        tf->imageCaptureDateTime = dt;
                }
            }
        } else {
            uncachedPaths_ << item.fileInfo.absoluteFilePath();
        }
    }
    emit scanProgress(tagged_files_->rowCount());
}

/*! \brief Called when the background FolderScanner finishes; kicks off icon generation. */
void CompendiaCore::onScanFinished()
{
    // The thread and scanner objects will self-destruct via deleteLater connections;
    // null the pointers so cancelIconGeneration() doesn't try to use them again.
    folderScanner_ = nullptr;
    scanThread_    = nullptr;

    int toCache = uncachedPaths_.size();
    backfillMetadata();
    tagged_files_proxy_->sort(0);
    updateWantedPaths();   // seed visible-icon set so first paint schedules loads without a scroll/resize event

    // Populate the filesystem watcher with all unique directories containing tracked files.
    if (fileWatcher_) {
        QSet<QString> dirs;
        dirs.insert(root_directory_);
        for (int i = 0; i < tagged_files_->rowCount(); ++i) {
            auto tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
            dirs.insert(tf->filePath);  // filePath is already the containing directory
        }
        dirs.removeIf([](const QString& d) {
            return d.contains(QLatin1String(Compendia::CacheFolderName));
        });
        fileWatcher_->addPaths(QStringList(dirs.begin(), dirs.end()));
    }

    emit scanFinished(tagged_files_->rowCount(), toCache);
    loadTagLibraryFile();
}

/*! \brief Starts the UI flush timer if it is not already running. */
void CompendiaCore::ensureUiFlushTimerRunning(){
    if (!uiFlushTimer_.isActive()) uiFlushTimer_.start();
}

/*! \brief Returns true if any file in the model has unsaved changes.
 *
 * \return True when at least one TaggedFile has a dirty flag set.
 */
bool CompendiaCore::hasUnsavedChanges()
{
    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        if (tagged_files_->item(row)->data().value<TaggedFile*>()->dirtyFlag())
            return true;
    }
    return false;
}

/*! \brief Writes JSON sidecar files for every file with unsaved changes. */
void CompendiaCore::writeFileMetadata(){

    int dirtyCount = 0;
    for (int row = 0; row < tagged_files_->rowCount(); ++row)
        if (tagged_files_->item(row)->data().value<TaggedFile*>()->dirtyFlag())
            ++dirtyCount;
    qDebug() << "[CompendiaCore] saving — dirty file count:" << dirtyCount;

    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        QVariant fi = tagged_files_->item(row)->data();
        TaggedFile* itemAsTaggedFile = fi.value<TaggedFile*>();

        if (itemAsTaggedFile->dirtyFlag()) {
            QString origFile = itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName;
            QFileInfo fileInfo(origFile);
            QString metaFilePath = itemAsTaggedFile->filePath + "/" + fileInfo.completeBaseName() + ".json";

            QFile metaFile(metaFilePath);

            if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qDebug() << "Could not open file for writing:" << metaFile.errorString();
            }

            QTextStream out(&metaFile);
            out << itemAsTaggedFile->TaggedFileJSON();
            metaFile.close();
        }

        emit metadataSaved();
    }

    clearAllDirtyFlags();
}

/*! \brief Writes JSON sidecar files for every visible (proxy-filtered) file with unsaved changes.
 *
 * Non-visible files that are dirty via a tag or tag-family rename are explicitly promoted to
 * file-level dirty before the global tag/family flags are cleared, so they remain dirty and will
 * be picked up by the next Save All.
 */
void CompendiaCore::writeVisibleFileMetadata()
{
    // Collect the set of source rows that are visible so we can exclude them below.
    QSet<int> visibleSourceRows;
    for (int row = 0; row < tagged_files_proxy_->rowCount(); ++row) {
        QModelIndex srcIndex = tagged_files_proxy_->mapToSource(tagged_files_proxy_->index(row, 0));
        visibleSourceRows.insert(srcIndex.row());
    }

    // Promote non-visible dirty files to file-level dirty before we clear the tag/family flags.
    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        if (visibleSourceRows.contains(row)) continue;
        TaggedFile* tf = tagged_files_->item(row)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf->dirtyFlag())
            tf->markDirty();
    }

    for (int row = 0; row < tagged_files_proxy_->rowCount(); ++row) {
        QModelIndex proxyIndex = tagged_files_proxy_->index(row, 0);
        QModelIndex srcIndex   = tagged_files_proxy_->mapToSource(proxyIndex);
        TaggedFile* itemAsTaggedFile = tagged_files_->itemFromIndex(srcIndex)->data(Qt::UserRole + 1).value<TaggedFile*>();

        if (itemAsTaggedFile->dirtyFlag()) {
            QString origFile = itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName;
            QFileInfo fileInfo(origFile);
            QString metaFilePath = itemAsTaggedFile->filePath + "/" + fileInfo.completeBaseName() + ".json";

            QFile metaFile(metaFilePath);

            if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qDebug() << "Could not open file for writing:" << metaFile.errorString();
            }

            QTextStream out(&metaFile);
            out << itemAsTaggedFile->TaggedFileJSON();
            metaFile.close();
            itemAsTaggedFile->clearDirtyFlag();
        }

        emit metadataSaved();
    }

    // Clear only the tag and family dirty flags — file-level flags are handled per-file above
    // (visible files' flags are cleared by clearAllDirtyFlags below after Save All, but here we
    // need a targeted clear so that non-visible files promoted to file-level dirty are not lost).
    for (Tag* tag : *tags_)
        tag->clearDirtyFlag();
    for (TagFamily* family : *tag_families_)
        family->clearDirtyFlag();
}

/*! \brief Clears the dirty flag on every file, tag, and tag family. */
void CompendiaCore::clearAllDirtyFlags()
{
    for (int row = 0; row < tagged_files_->rowCount(); ++row)
        tagged_files_->item(row)->data().value<TaggedFile*>()->clearDirtyFlag();
    for (Tag* tag : *tags_)
        tag->clearDirtyFlag();
    for (TagFamily* family : *tag_families_)
        family->clearDirtyFlag();
}

/*! \brief Writes all library tags and families to _compendia_tag_library.json at the root folder.
 *
 * No-op until libraryFileInitialized_ is true (prevents spurious writes during scan loading).
 * Triggered automatically via the tagLibraryChanged signal and Tag/TagFamily::nameChanged connections.
 */
void CompendiaCore::writeTagLibraryFile()
{
    if (!libraryFileInitialized_ || root_directory_.isEmpty()) return;

    QJsonObject families;
    for (Tag* tag : std::as_const(*tags_)) {
        QString fn = tag->tagFamily->getName();
        if (!families.contains(fn)) {
            QJsonObject famObj;
            famObj.insert("color_index", tag->tagFamily->getColorIndex());
            famObj.insert("tags", QJsonArray());
            families.insert(fn, famObj);
        }
        QJsonObject famObj = families[fn].toObject();
        QJsonArray arr = famObj["tags"].toArray();
        arr.append(tag->getName());
        famObj["tags"] = arr;
        families[fn] = famObj;
    }
    // Include families that have no tags
    for (TagFamily* fam : std::as_const(*tag_families_)) {
        if (!families.contains(fam->getName())) {
            QJsonObject famObj;
            famObj.insert("color_index", fam->getColorIndex());
            famObj.insert("tags", QJsonArray());
            families.insert(fam->getName(), famObj);
        }
    }

    QJsonObject root;
    root.insert("compendia_version", APP_VERSION_STRING);
    root.insert("next_color_index", TagFamily::getNextHue());
    root.insert("tags", families);

    QFile file(root_directory_ + "/_compendia_tag_library.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[CompendiaCore] Could not write tag library:" << file.errorString();
        return;
    }
    QTextStream(&file) << QJsonDocument(root).toJson();
}

/*! \brief Reads _compendia_tag_library.json from the root folder and merges any tags not already
 *  in the library (e.g. tags the user created but never applied to a file).
 *
 * Sets libraryFileInitialized_ to true when done, enabling subsequent auto-saves.
 * Silently no-ops if the file does not exist or cannot be parsed.
 */
void CompendiaCore::loadTagLibraryFile()
{
    QString libPath = root_directory_ + "/_compendia_tag_library.json";
    QFile file(libPath);
    int storedNextHue = -1;
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isNull()) {
            QJsonObject rootObj = doc.object();
            if (rootObj.contains("next_color_index"))
                storedNextHue = rootObj["next_color_index"].toInt(-1);

            QJsonObject tagsObj = rootObj["tags"].toObject();
            for (auto it = tagsObj.begin(); it != tagsObj.end(); ++it) {
                QString familyName = it.key();
                QJsonValue val = it.value();

                if (val.isObject()) {
                    // New format: { "color_index": N, "tags": [...] }
                    QJsonObject famObj = val.toObject();
                    int colorIndex = famObj["color_index"].toInt(-1);
                    QJsonArray tagArr = famObj["tags"].toArray();

                    // Find or create family using the stored colour index
                    TagFamily* fam = nullptr;
                    for (TagFamily* f : std::as_const(*tag_families_)) {
                        if (f->getName() == familyName) { fam = f; break; }
                    }
                    if (!fam) {
                        fam = (colorIndex >= 0)
                            ? new TagFamily(familyName, colorIndex, this)
                            : new TagFamily(familyName, this);
                        tag_families_->insert(fam);
                        connect(fam, &TagFamily::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
                    }
                    for (const QJsonValue& v : tagArr)
                        addLibraryTag(familyName, v.toString());

                } else if (val.isArray()) {
                    // Old format: bare array — fall back to auto-generated colour
                    QJsonArray tagArr = val.toArray();
                    if (tagArr.isEmpty())
                        addLibraryTagFamily(familyName);
                    else
                        for (const QJsonValue& v : tagArr)
                            addLibraryTag(familyName, v.toString());
                }
            }
        }
    }
    // Restore the sequence counter so new families continue from where the last session left off
    if (storedNextHue >= 0)
        TagFamily::setNextHue(storedNextHue);

    libraryFileInitialized_ = true;
}

/*! \brief Returns a pointer to the complete tag library.
 *
 * \return Pointer to the set of all known Tag objects.
 */
QSet<Tag*>* CompendiaCore::getLibraryTags(){
    return tags_;
}

/*! \brief Returns a new set of tags that are assigned to at least one file.
 *
 * \return Heap-allocated set of assigned Tag pointers (caller takes ownership).
 */
QSet<Tag*>* CompendiaCore::getAssignedTags(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    QSet<Tag*>* distinctTags = new QSet<Tag*>();

    for (int i = 0; i < tagged_files_->rowCount(); ++i){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        QSet<Tag*> &tgs = *itemAsTaggedFile->tags();
        distinctTags->unite(tgs);
    }
    return distinctTags;
}

/*! \brief Returns a new set of tags assigned to any currently visible (filtered) file.
 *
 * \return Heap-allocated set of Tag pointers (caller takes ownership).
 */
QSet<Tag*>* CompendiaCore::getAssignedTags_FilteredFiles(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a visible file
    QSet<Tag*>* distinctTags = new QSet<Tag*>();

    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){

        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = tagged_files_proxy_->mapToSource(proxyIndex);

        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

        QSet<Tag*> &tgs = *itemAsTaggedFile->tags();
        distinctTags->unite(tgs);
    }
    return distinctTags;
}

/*! \brief Returns a pointer to the set of tags currently active as filters.
 *
 * \return Pointer to the proxy model's internal filter tag set.
 */
QSet<Tag*>* CompendiaCore::getFilterTags(){
    return tagged_files_proxy_->getFilterTags();
}

/*! \brief Retrieve a tag from the library by name.
 *
 * \param tagFamilyName The name of the tag family to search within.
 * \param tagName       The tag name to find.
 * \return Pointer to the matching Tag, or nullptr if not found.
 */
Tag* CompendiaCore::getTag(QString tagFamilyName, QString tagName){
    Tag* matchingTag = nullptr;

    QSetIterator<Tag *> i(*tags_);
    while (i.hasNext()) {
        Tag* t = i.next();
        if ( t->getName() == tagName && t->tagFamily->getName() == tagFamilyName){
            matchingTag = t;
            break;
        }
    }

    return matchingTag;
}

/*! \brief Retrieve a tag family from the library by name.
 *
 * \param tagFamilyName The family name to find.
 * \return Pointer to the matching TagFamily, or nullptr if not found.
 */
TagFamily* CompendiaCore::getTagFamily(QString tagFamilyName){
    TagFamily* matchingTagFamily = nullptr;

    QSetIterator<TagFamily *> i(*tag_families_);
    while (i.hasNext()) {
        TagFamily* tf = i.next();
        if ( tf->getName() == tagFamilyName){
            matchingTagFamily = tf;
            break;
        }
    }

    return matchingTagFamily;
}

/*! \brief Create a tag in the library using just its family name and tag name as strings.
 *
 *  \param tagFamilyName The name of the family the tag belongs to.
 *  \param tagName       The name of the tag to create or find.
 *  \return Pointer to the new or existing Tag.
 */
Tag* CompendiaCore::addLibraryTag(QString tagFamilyName, QString tagName){

    TagFamily* tf = addLibraryTagFamily(tagFamilyName);
    Tag* matchingTag = nullptr;
    Tag* t;

    QSetIterator<Tag *> i(*tags_);
    while (i.hasNext()) {
        Tag* t = i.next();
        if(t->getName() == tagName && t->tagFamily == tf){
            matchingTag = t;
            break;
        }
    }
    if(matchingTag == nullptr){
        matchingTag = new Tag(tf, tagName, this);
        tags_->insert(matchingTag);
        connect(matchingTag, &Tag::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
    }
    emit tagLibraryChanged();
    return matchingTag;
}

/*! \brief Add a tag to the library if it doesn't exist.
 *
 * Transfers Qt ownership of \p tag to this CompendiaCore instance so that the
 * tag is not accidentally destroyed when the UI widget that originally created
 * it (typically a TagFamilyWidget) is rebuilt and its children are deleted.
 *
 * \param tag The Tag pointer to insert.
 */
void CompendiaCore::addLibraryTag(Tag* tag){
    tag->setParent(this);  // Take ownership: prevent widget destruction from deleting the tag
    tags_->insert(tag);
    connect(tag, &Tag::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
    emit tagLibraryChanged();
}

/*! \brief Creates or retrieves a tag family in the library by name.
 *
 * \param tagFamilyName The family name to create or find.
 * \return Pointer to the new or existing TagFamily.
 */
TagFamily* CompendiaCore::addLibraryTagFamily(QString tagFamilyName){

    TagFamily* matchingFam = nullptr;
    TagFamily* fam = nullptr;

    QSetIterator<TagFamily *> i(*tag_families_);
    while (i.hasNext()) {
        TagFamily* fam = i.next();
        if(fam->getName() == tagFamilyName){
            matchingFam = fam;
            break;
        }
    }
    if(matchingFam == nullptr){
        matchingFam = new TagFamily(tagFamilyName, this);
        tag_families_->insert(matchingFam);
        connect(matchingFam, &TagFamily::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
    }
    writeTagLibraryFile();
    return matchingFam;
}

/*! \brief Adds an existing TagFamily pointer to the library if not already present.
 *
 * \param tagFamily The TagFamily pointer to insert.
 */
void CompendiaCore::addLibraryTagFamily(TagFamily* tagFamily){
    if(! tag_families_->contains(tagFamily)){
        tag_families_->insert(tagFamily);
        connect(tagFamily, &TagFamily::nameChanged, this, &CompendiaCore::writeTagLibraryFile);
        writeTagLibraryFile();
    }
}

/*! \brief Applies a tag to all files currently visible in the filtered file list.
 *
 * \param tag The Tag to apply.
 */
void CompendiaCore::applyTag(Tag* tag){
    // Apply tag to all the files in the filtered file list
    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){
        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = tagged_files_proxy_->mapToSource(proxyIndex);
        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        itemAsTaggedFile->addTag(tag);
    }
}

/*! \brief Applies the tag identified by a TagSet to a specific file.
 *
 * \param f The TaggedFile to tag.
 * \param t The TagSet identifying the tag to apply.
 */
void CompendiaCore::applyTag(TaggedFile* f, TagSet t){
    Tag* tag = getTag(t.tagFamilyName, t.tagName);
    f->addTag(tag);
}

/*! \brief Removes a tag from all files currently visible in the filtered file list.
 *
 * \param tag The Tag to remove.
 */
void CompendiaCore::unapplyTag(Tag* tag){
    // Remove tag from all the files in the filtered file list
    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){
        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = tagged_files_proxy_->mapToSource(proxyIndex);
        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        itemAsTaggedFile->removeTag(tag);
    }
}

/*! \brief Returns the number of currently visible files that have the given tag assigned.
 *
 * \param tag The Tag to count.
 * \return Count of visible files carrying the tag.
 */
int CompendiaCore::countVisibleFilesWithTag(Tag* tag){
    int count = 0;
    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i) {
        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);
        QModelIndex sourceIndex = tagged_files_proxy_->mapToSource(proxyIndex);
        TaggedFile* tf = sourceIndex.data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf && tf->tags()->contains(tag))
            ++count;
    }
    return count;
}

/*! \brief Returns the number of all files (ignoring the proxy filter) that have the given tag.
 *
 * \param tag The Tag to count.
 * \return Count of all files carrying the tag.
 */
int CompendiaCore::countAllFilesWithTag(Tag* tag){
    int count = 0;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile* tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf && tf->tags()->contains(tag))
            ++count;
    }
    return count;
}

/*! \brief Removes a tag from all files, the filter, and the library, then deletes it.
 *
 * Emits tagLibraryChanged() when done.
 * \param tag The Tag to delete.
 */
void CompendiaCore::deleteTagFromLibrary(Tag* tag){
    TagFamily* family = tag->tagFamily;

    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile* tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf)
            tf->removeTag(tag);
    }
    removeTagFilter(tag);
    tags_->remove(tag);
    tag->deleteLater();

    // If no other library tags belong to this family, remove the family too
    const bool familyEmpty = std::none_of(tags_->cbegin(), tags_->cend(),
                                          [family](Tag* t){ return t->tagFamily == family; });
    if (familyEmpty) {
        tag_families_->remove(family);
        family->deleteLater();
    }

    emit tagLibraryChanged();
}

/*! \brief Removes a tag from a specific file.
 *
 * \param file The TaggedFile to remove the tag from.
 * \param tag  The Tag to remove.
 */
void CompendiaCore::unapplyTag(TaggedFile* file, Tag* tag){
    // Locate the file and remove the indicated tag from it
    for( int i = 0; i < tagged_files_->rowCount(); ++i ){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        if(itemAsTaggedFile == file)
            itemAsTaggedFile->removeTag(tag);
    }
}

/*! \brief Returns the underlying QStandardItemModel containing all file items.
 *
 * \return Pointer to the source item model.
 */
QStandardItemModel* CompendiaCore::getItemModel(){
    return tagged_files_;
}

/*! \brief Returns the FilterProxyModel used by the file list view.
 *
 * \return Pointer to the proxy model.
 */
FilterProxyModel* CompendiaCore::getItemModelProxy(){
    return tagged_files_proxy_;
}

/*! \brief Sets the filename substring filter on the proxy model.
 *
 * \param filterText The substring to match against file names.
 */
void CompendiaCore::setFileNameFilter(QString filterText){
    tagged_files_proxy_->setNameFilter(filterText);
}

/*! \brief Sets the folder path substring filter on the proxy model.
 *
 * \param filterText The substring to match against folder paths.
 */
void CompendiaCore::setFolderFilter(QString filterText){
    tagged_files_proxy_->setFolderFilter(filterText);
}

/*! \brief Sets the creation-date filter on the proxy model.
 *
 * \param date The date to filter by; an invalid QDate clears the filter.
 */
void CompendiaCore::setCreationDateFilter(QDate date) {
    tagged_files_proxy_->setFilterCreationDate(date);
}

/*! \brief Sets the rating filter on the proxy model.
 *
 * \param rating The rating to filter for [1,5], or std::nullopt to disable.
 */
void CompendiaCore::setRatingFilter(std::optional<int> rating) {
    tagged_files_proxy_->setRatingFilter(rating);
}

/*! \brief Sets the comparison mode used when evaluating the rating filter.
 *
 * \param mode LessOrEqual, Exactly, or GreaterOrEqual.
 */
void CompendiaCore::setRatingFilterMode(FilterProxyModel::RatingFilterMode mode) {
    tagged_files_proxy_->setRatingFilterMode(mode);
}

/*! \brief Returns a chronologically sorted list of unique effective dates across all loaded files.
 *
 * Each file's effective date is its EXIF capture date when present, falling back to the
 * filename-inferred date, then the filesystem creation date.
 * Used to populate the date-filter autocomplete.
 */
QList<QDate> CompendiaCore::getFileDates() const
{
    QSet<QDate> seen;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile *tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;
        QDate d = tf->effectiveDate();
        if (d.isValid())
            seen.insert(d);
    }
    QList<QDate> sorted = QList<QDate>(seen.cbegin(), seen.cend());
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

/*! \brief Returns a sorted list of unique folder paths across all loaded files.
 *
 * Used to populate the folder-filter autocomplete.
 */
QList<QString> CompendiaCore::getFileFolders() const
{
    QSet<QString> seen;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile *tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf || tf->filePath.isEmpty()) continue;
        seen.insert(tf->filePath);
    }
    QList<QString> sorted = QList<QString>(seen.cbegin(), seen.cend());
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

/*! \brief Adds a tag to the active tag filter set.
 *
 * \param tag The Tag to add to the filter.
 */
void CompendiaCore::addTagFilter(Tag* tag){
    tagged_files_proxy_->addTagFilter(tag);
}

/*! \brief Removes a tag from the active tag filter set.
 *
 * \param tag The Tag to remove from the filter.
 */
void CompendiaCore::removeTagFilter(Tag* tag){
    tagged_files_proxy_->removeTagFilter(tag);
}

/*! \brief Clears all active tag filters. */
void CompendiaCore::clearAllTagFilters()
{
    tagged_files_proxy_->clearTagFilters();
}

/*! \brief Restricts the visible set to the given files; other filters still apply within the set.
 *
 * \param files The set of TaggedFile pointers to isolate.
 */
void CompendiaCore::setIsolationSet(const QSet<TaggedFile*> &files) {
    tagged_files_proxy_->setIsolationSet(files);
}

/*! \brief Clears the isolation set so all files are eligible again.
 */
void CompendiaCore::clearIsolationSet() {
    tagged_files_proxy_->clearIsolationSet();
}

/*! \brief Returns true when an isolation set is active.
 *
 * \return True if the proxy model has a non-empty isolation set.
 */
bool CompendiaCore::isIsolated() const {
    return tagged_files_proxy_->isIsolated();
}

/*! \brief Returns the number of files in the current isolation set.
 *
 * \return Size of the isolation set, or 0 when not isolated.
 */
int CompendiaCore::isolationSetSize() const {
    return tagged_files_proxy_->isolationSetSize();
}

/*! \brief Groups all files whose pHash values are within \a threshold Hamming distance of each other.
 *
 * Uses a path-compressed Union-Find algorithm over all files with a valid pHash.
 * Only groups of two or more files are returned.
 *
 * \param threshold Maximum Hamming distance to consider two images near-duplicates.
 * \return List of groups; each group contains two or more TaggedFile pointers.
 */
QList<QList<TaggedFile*>> CompendiaCore::findSimilarImages(int threshold)
{
    QList<TaggedFile*> files;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile *tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf && tf->pHash() != 0)
            files.append(tf);
    }

    const int n = files.size();
    QVector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);

    std::function<int(int)> find = [&](int x) -> int {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    };

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (PerceptualHasher::hammingDistance(files[i]->pHash(), files[j]->pHash()) <= threshold) {
                int pi = find(i), pj = find(j);
                if (pi != pj) parent[pi] = pj;
            }
        }
    }

    QMap<int, QList<TaggedFile*>> groups;
    for (int i = 0; i < n; ++i)
        groups[find(i)].append(files[i]);

    QList<QList<TaggedFile*>> result;
    for (const auto &g : groups)
        if (g.size() >= 2)
            result.append(g);

    return result;
}

/*! \brief Returns all files within \a threshold Hamming distance of any file in \a seeds.
 *
 * Includes the seeds themselves in the result.  Files with a zero pHash are skipped
 * as candidates (but seeds with a zero pHash are still included in the result set).
 *
 * \param seeds     The set of TaggedFile pointers to compare against.
 * \param threshold Maximum Hamming distance to consider a file similar to a seed.
 * \return Set of similar files including all seeds.
 */
QSet<TaggedFile*> CompendiaCore::findSimilarTo(const QSet<TaggedFile*> &seeds, int threshold)
{
    QSet<TaggedFile*> result = seeds;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        TaggedFile* candidate = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!candidate || candidate->pHash() == 0 || result.contains(candidate))
            continue;
        for (TaggedFile* seed : seeds) {
            if (seed->pHash() != 0 &&
                PerceptualHasher::hammingDistance(candidate->pHash(), seed->pHash()) <= threshold) {
                result.insert(candidate);
                break;
            }
        }
    }
    return result;
}

/*! \brief Partitions \a files into connected components by pHash similarity.
 *
 * Applies the same union-find algorithm as findSimilarImages() but operates on
 * an arbitrary caller-supplied set rather than the full model.
 * Only groups with two or more files are returned.
 *
 * \param files     The set of TaggedFile pointers to partition.
 * \param threshold Maximum Hamming distance to consider two images near-duplicates.
 * \return List of groups; each group contains two or more TaggedFile pointers.
 */
QList<QList<TaggedFile*>> CompendiaCore::groupBySimilarity(
        const QSet<TaggedFile*> &files, int threshold) const
{
    QList<TaggedFile*> list = files.values();
    const int n = list.size();
    QVector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);

    std::function<int(int)> find = [&](int x) -> int {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    };

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (list[i]->pHash() != 0 && list[j]->pHash() != 0 &&
                PerceptualHasher::hammingDistance(list[i]->pHash(), list[j]->pHash()) <= threshold) {
                int pi = find(i), pj = find(j);
                if (pi != pj) parent[pi] = pj;
            }
        }
    }

    QMap<int, QList<TaggedFile*>> groups;
    for (int i = 0; i < n; ++i)
        groups[find(i)].append(list[i]);

    QList<QList<TaggedFile*>> result;
    for (const auto &g : groups)
        if (g.size() >= 2)
            result.append(g);
    return result;
}

/*! \brief Removes all auto-detected face tags from every file, the active
 *  filter, and the tag library. Cleans up now-empty tag families and emits
 *  tagLibraryChanged() once when done.
 *
 * \return Number of tags removed, or 0 if there were none.
 */
int CompendiaCore::removeAutoDetectedFaceTags()
{
    // Collect auto tags from the library into a snapshot
    QList<Tag*> autoTags;
    for (Tag* t : *tags_) {
        if (t->getName().startsWith(Compendia::AutoFaceTagPrefix))
            autoTags.append(t);
    }

    if (autoTags.isEmpty())
        return 0;

    // Remove auto tags from every file
    for (int r = 0; r < tagged_files_->rowCount(); ++r) {
        TaggedFile* tf = tagged_files_->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;
        for (Tag* t : autoTags)
            tf->removeTag(t);
    }

    // Remove from filter, library, and clean up now-empty families
    for (Tag* t : autoTags) {
        TagFamily* family = t->tagFamily;
        removeTagFilter(t);
        tags_->remove(t);

        // Remove family if no remaining library tags belong to it
        const bool familyEmpty = std::none_of(tags_->cbegin(), tags_->cend(),
                                              [family](Tag* u){ return u->tagFamily == family; });
        if (familyEmpty) {
            tag_families_->remove(family);
            family->deleteLater();
        }

        t->deleteLater();
    }

    emit tagLibraryChanged();
    return autoTags.size();
}

/*! \brief Merges \a from into \a into across all files, then removes and schedules \a from for deletion.
 *  Emits tagLibraryChanged() when done.
 *
 * \param from The Tag to be removed after merging.
 * \param into The Tag that survives the merge and receives all file/filter references.
 */
void CompendiaCore::mergeTag(Tag* from, Tag* into)
{
    // Re-route every file that carries 'from'
    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        QStandardItem* item = tagged_files_->item(row);
        TaggedFile* tf = item->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf || !tf->tags()->contains(from))
            continue;
        // Transfer the bounding rect if 'into' has none yet
        auto fromRect = tf->tagRect(from);
        tf->removeTag(from);
        if (!tf->tags()->contains(into)) {
            if (fromRect.has_value())
                tf->addTag(into, fromRect.value());
            else
                tf->addTag(into);
        }
    }

    // Replace 'from' in the active filter set if it is a filter
    if (getFilterTags()->contains(from)) {
        removeTagFilter(from);
        if (!getFilterTags()->contains(into))
            addTagFilter(into);
    }

    tags_->remove(from);
    from->deleteLater();
    emit tagLibraryChanged();
}

/*! \brief Merges all tags of family \a from into family \a into, handling per-tag collisions
 *  recursively, then removes and schedules \a from for deletion. Emits tagLibraryChanged().
 *
 * \param from The TagFamily to be removed after merging.
 * \param into The TagFamily that survives and receives all tags.
 */
void CompendiaCore::mergeTagFamily(TagFamily* from, TagFamily* into)
{
    // Iterate a snapshot because mergeTag may modify tags_
    QList<Tag*> tagSnapshot(tags_->begin(), tags_->end());
    for (Tag* t : tagSnapshot) {
        if (t->tagFamily != from)
            continue;
        // Look for a same-name tag already in 'into'
        Tag* collision = getTag(into->getName(), t->getName());
        if (collision) {
            mergeTag(t, collision);   // handles file re-routing and deletion
        } else {
            t->tagFamily = into;
            t->markDirty();
        }
    }

    tag_families_->remove(from);
    from->deleteLater();
    emit tagLibraryChanged();
}

/*! \brief Removes \a family from the library if no tags reference it any longer. */
void CompendiaCore::cleanupFamilyIfEmpty(TagFamily* family)
{
    for (Tag* t : std::as_const(*tags_)) {
        if (t->tagFamily == family)
            return;
    }
    tag_families_->remove(family);
    family->deleteLater();
}

/*! \brief Moves \a tag to \a newFamily, merging with a same-name tag if one already exists.
 *  Removes the old family if it becomes empty. Emits tagLibraryChanged(). */
void CompendiaCore::refamilyTag(Tag* tag, TagFamily* newFamily)
{
    if (!tag || !newFamily || tag->tagFamily == newFamily)
        return;

    TagFamily* oldFamily = tag->tagFamily;
    Tag* collision = getTag(newFamily->getName(), tag->getName());
    if (collision) {
        mergeTag(tag, collision);   // re-routes files, emits tagLibraryChanged
    } else {
        tag->setTagFamily(newFamily);
    }

    cleanupFamilyIfEmpty(oldFamily);
    emit tagLibraryChanged();
}

/*! \brief Parses a JSON object of tag-family to tag-name arrays into a TagSet list.
 *
 * \param tagsJson     A JSON object mapping family name strings to arrays of tag name strings.
 * \param tagRectsJson Optional JSON object mapping family → tag → [x,y,w,h] for bounding rects.
 * \return A list of TagSet values parsed from the JSON.
 */
QList<TagSet> CompendiaCore::parseTagJson(QJsonObject tagsJson, QJsonObject tagRectsJson){
    QList<TagSet> tagSets;

    for (auto it = tagsJson.begin(); it != tagsJson.end(); ++it) {
        QString familyName = it.key();
        QJsonArray array = it.value().toArray();
        for (const QJsonValue &value : array){
            QString tagName = value.toString();
            TagSet ts(familyName, tagName);

            if (tagRectsJson.contains(familyName)) {
                QJsonObject familyRects = tagRectsJson[familyName].toObject();
                if (familyRects.contains(tagName)) {
                    QJsonArray rectArr = familyRects[tagName].toArray();
                    if (rectArr.size() == 4) {
                        ts.rect = QRectF(rectArr[0].toDouble(), rectArr[1].toDouble(),
                                         rectArr[2].toDouble(), rectArr[3].toDouble());
                    }
                }
            }

            tagSets.append(ts);
        }
    }

    return tagSets;
}

/*! \brief Called when a watched directory's contents change on disk. */
void CompendiaCore::onWatchedDirectoryChanged(const QString& path)
{
    pendingChangedDirs_.insert(path);
    watcherDebounceTimer_.start();  // restarts if already running
}

/*! \brief Called when a watched file is modified or deleted on disk. */
void CompendiaCore::onWatchedFileChanged(const QString& path)
{
    Q_UNUSED(path)
    // Individual file watches are not currently used; directory diffs cover removal detection.
}

/*! \brief Diffs disk contents of \a dirPath against the model; returns appeared and disappeared paths. */
CompendiaCore::DirDiff CompendiaCore::diffDirectory(const QString& dirPath) const
{
    qDebug() << "[FileWatch] diffDirectory:" << dirPath;

    static const QStringList nameFilters = {
        "*.jpg",  "*.JPG",  "*.heic", "*.HEIC", "*.png",  "*.PNG",
        "*.mp4",  "*.MP4",  "*.mov",  "*.MOV",  "*.avi",  "*.AVI",
        "*.mkv",  "*.MKV",  "*.wmv",  "*.WMV",  "*.webm", "*.WEBM",
        "*.m4v",  "*.M4V"
    };

    // Filenames currently on disk in this directory (excluding .trashed)
    QSet<QString> onDisk;
    for (const QString& name : QDir(dirPath).entryList(nameFilters, QDir::Files)) {
        if (!name.startsWith(".trashed", Qt::CaseInsensitive))
            onDisk.insert(name);
    }

    // Filenames the model knows about for this directory
    QSet<QString> inModel;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        auto tf = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf && tf->filePath == dirPath)
            inModel.insert(tf->fileName);
    }

    DirDiff diff;
    for (const QString& name : onDisk)
        if (!inModel.contains(name))
            diff.appeared.append(dirPath + "/" + name);
    for (const QString& name : inModel)
        if (!onDisk.contains(name))
            diff.disappeared.append(dirPath + "/" + name);

    return diff;
}

/*! \brief Flushes pendingChangedDirs_, correlates appeared/disappeared pairs as moves, dispatches handlers. */
void CompendiaCore::processWatcherChanges()
{
    qDebug() << "[FileWatch] processWatcherChanges: flushing" << pendingChangedDirs_.size() << "dir(s)";

    QStringList allAppeared;
    QStringList allDisappeared;

    for (const QString& dir : pendingChangedDirs_) {
        DirDiff diff = diffDirectory(dir);
        allAppeared.append(diff.appeared);
        allDisappeared.append(diff.disappeared);
    }
    pendingChangedDirs_.clear();

    // Correlate same-filename disappearances and appearances as moves within the watched tree
    QStringList unmatched;
    for (const QString& oldPath : allDisappeared) {
        const QString fileName = QFileInfo(oldPath).fileName();
        auto it = std::find_if(allAppeared.begin(), allAppeared.end(),
                               [&](const QString& p){ return QFileInfo(p).fileName() == fileName; });
        if (it != allAppeared.end()) {
            handleFileMoved(oldPath, *it);
            allAppeared.erase(it);
        } else {
            unmatched.append(oldPath);
        }
    }

    for (const QString& path : unmatched)
        handleFileRemoved(path);

    for (const QString& path : allAppeared)
        handleFileAdded(path);
}

/*! \brief Handles a file that disappeared with no matching appearance elsewhere in the watched tree. */
void CompendiaCore::handleFileRemoved(const QString& absolutePath)
{
    qDebug() << "[FileWatch] handleFileRemoved:" << absolutePath;

    // Locate the model entry
    TaggedFile* tf = nullptr;
    int foundRow = -1;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        auto candidate = tagged_files_->item(i)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (candidate && candidate->filePath + "/" + candidate->fileName == absolutePath) {
            tf = candidate;
            foundRow = i;
            break;
        }
    }

    if (!tf) {
        qDebug() << "[FileWatch] handleFileRemoved: no model entry found, ignoring";
        return;
    }

    // Only handle the clean case for now; dirty files need user interaction (not yet implemented)
    // TODO: come back and think through lost changes in sidecar
    if (tf->dirtyFlag()) {
        qDebug() << "[FileWatch] handleFileRemoved: file has unsaved changes, skipping (not yet handled)";
        return;
    }

    // Delete sidecar if one exists alongside the (now-gone) image
    const QString sidecarPath = tf->filePath + "/" + QFileInfo(tf->fileName).completeBaseName() + ".json";
    if (QFile::exists(sidecarPath)) {
        if (QFile::remove(sidecarPath))
            qDebug() << "[FileWatch] handleFileRemoved: deleted sidecar" << sidecarPath;
        else
            qDebug() << "[FileWatch] handleFileRemoved: failed to delete sidecar" << sidecarPath;
    }

    // Delete icon cache files (one per thumbnail size) and the EXIF cache
    for (int size : IconGenerator::kIconSizes) {
        const QString iconCache = IconGenerator::cacheFilePath(absolutePath, size);
        if (QFile::exists(iconCache) && !QFile::remove(iconCache))
            qDebug() << "[FileWatch] handleFileRemoved: failed to delete icon cache" << iconCache;
    }
    const QString exifCache = QFileInfo(absolutePath).absolutePath() + "/"
                              + Compendia::CacheFolderName + "/"
                              + QFileInfo(absolutePath).completeBaseName() + "_exif.json";
    if (QFile::exists(exifCache) && !QFile::remove(exifCache))
        qDebug() << "[FileWatch] handleFileRemoved: failed to delete EXIF cache" << exifCache;

    // Emit before removing so the pointer is still valid when MainWindow receives it
    emit fileRemovedExternally(tf, false, false);

    tagged_files_->removeRow(foundRow);
    delete tf;

    qDebug() << "[FileWatch] handleFileRemoved: removed from model";
}

/*! \brief Handles a file that moved from one watched directory to another. */
void CompendiaCore::handleFileMoved(const QString& oldPath, const QString& newPath)
{
    qDebug() << "[FileWatch] handleFileMoved:" << oldPath << "->" << newPath;

    // Locate the model entry by old absolute path
    TaggedFile* tf = nullptr;
    QStandardItem* tfItem = nullptr;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        QStandardItem* item = tagged_files_->item(i);
        auto candidate = item->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (candidate && candidate->filePath + "/" + candidate->fileName == oldPath) {
            tf = candidate;
            tfItem = item;
            break;
        }
    }

    if (!tf) {
        qDebug() << "[FileWatch] handleFileMoved: no model entry for oldPath, ignoring";
        return;
    }

    const QFileInfo oldInfo(oldPath);
    const QFileInfo newInfo(newPath);
    const QString oldSidecar = oldInfo.absolutePath() + "/" + oldInfo.completeBaseName() + ".json";
    const QString newSidecar = newInfo.absolutePath() + "/" + newInfo.completeBaseName() + ".json";

    // Move sidecar if it was left behind at the old location
    bool sidecarStranded = false;
    if (QFile::exists(oldSidecar)) {
        QDir().mkpath(newInfo.absolutePath());
        if (QFile::rename(oldSidecar, newSidecar))
            qDebug() << "[FileWatch] handleFileMoved: moved sidecar" << oldSidecar << "->" << newSidecar;
        else {
            sidecarStranded = true;
            qDebug() << "[FileWatch] handleFileMoved: failed to move sidecar" << oldSidecar;
        }
    }

    // Move icon cache files (one per thumbnail size)
    for (int size : IconGenerator::kIconSizes) {
        const QString oldIcon = IconGenerator::cacheFilePath(oldPath, size);
        const QString newIcon = IconGenerator::cacheFilePath(newPath, size);
        if (QFile::exists(oldIcon)) {
            QDir().mkpath(QFileInfo(newIcon).absolutePath());
            if (!QFile::rename(oldIcon, newIcon))
                qDebug() << "[FileWatch] handleFileMoved: failed to move icon cache" << oldIcon;
        }
    }

    // Move EXIF cache
    const QString oldExif = oldInfo.absolutePath() + "/" + Compendia::CacheFolderName + "/"
                            + oldInfo.completeBaseName() + "_exif.json";
    const QString newExif = newInfo.absolutePath() + "/" + Compendia::CacheFolderName + "/"
                            + newInfo.completeBaseName() + "_exif.json";
    if (QFile::exists(oldExif)) {
        QDir().mkpath(QFileInfo(newExif).absolutePath());
        if (!QFile::rename(oldExif, newExif))
            qDebug() << "[FileWatch] handleFileMoved: failed to move EXIF cache" << oldExif;
    }

    // Update TaggedFile in-memory and the model item display text
    tf->filePath = newInfo.absolutePath();
    tf->fileName = newInfo.fileName();
    tfItem->setText(newInfo.fileName());

    // Watch the new directory if not already watched
    if (fileWatcher_ && !fileWatcher_->directories().contains(newInfo.absolutePath()))
        fileWatcher_->addPath(newInfo.absolutePath());

    // Last-ditch safety: if no sidecar exists at the new location, mark dirty so
    // the user will be prompted to save and the data won't be silently lost.
    if (!QFile::exists(newSidecar))
        tf->markDirty();

    emit fileMovedExternally(tf, oldPath, sidecarStranded);
    qDebug() << "[FileWatch] handleFileMoved: model updated";
}

/*! \brief Handles a file that appeared in a watched directory with no matching disappearance elsewhere. */
void CompendiaCore::handleFileAdded(const QString& absolutePath)
{
    qDebug() << "[FileWatch] handleFileAdded:" << absolutePath;

    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
        return;

    // Load sidecar JSON if one exists alongside the new file
    const QString sidecarPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".json";
    QFile sidecarFile(sidecarPath);
    if (sidecarFile.exists() && sidecarFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonObject tagsJson = QJsonDocument::fromJson(sidecarFile.readAll()).object();
        sidecarFile.close();
        addFile(fileInfo, tagsJson);
        qDebug() << "[FileWatch] handleFileAdded: loaded with sidecar";
    } else {
        addFile(fileInfo);
    }

    // Apply cached EXIF immediately if the cache is already valid, otherwise schedule generation
    if (IconGenerator::iconCacheValid(absolutePath)) {
        QMap<QString, QString> cachedExif = ExifParser::getExifMap(absolutePath);
        QStandardItem *si = tagged_files_->item(tagged_files_->rowCount() - 1);
        if (si && !cachedExif.isEmpty()) {
            TaggedFile *tf = si->data(Qt::UserRole + 1).value<TaggedFile*>();
            if (tf) {
                tf->initExifMap(cachedExif);
                QDateTime dt = QDateTime::fromString(cachedExif["DateTime"], "yyyy:MM:dd HH:mm:ss");
                if (dt.isValid())
                    tf->imageCaptureDateTime = dt;
            }
        }
        qDebug() << "[FileWatch] handleFileAdded: icon cache valid, EXIF applied";
    } else if (iconGenerator_ == nullptr) {
        // No batch currently running — start one just for this file
        uiFlushTimer_.disconnect();
        connect(&uiFlushTimer_, &QTimer::timeout, this, &CompendiaCore::flushIconGeneratorQueue);
        uiFlushTimer_.setInterval(50);
        iconGenerator_ = new IconGenerator(this);
        connect(iconGenerator_, &IconGenerator::fileReady,  this, &CompendiaCore::onIconReady);
        connect(iconGenerator_, &IconGenerator::batchFinished, this, &CompendiaCore::onIconBatchFinished);
        iconGenerator_->processFiles({absolutePath});
        qDebug() << "[FileWatch] handleFileAdded: icon generation started";
    } else {
        // A batch is already in flight — queue this path and start a new batch when it finishes
        uncachedPaths_.append(absolutePath);
        connect(iconGenerator_, &IconGenerator::batchFinished, this, [this]() {
            if (!uncachedPaths_.isEmpty())
                backfillMetadata();
        }, Qt::SingleShotConnection);
        qDebug() << "[FileWatch] handleFileAdded: queued for icon generation after current batch";
    }

    // Watch the new file's directory if it isn't already (handles files dropped into new subdirs)
    if (fileWatcher_ && !fileWatcher_->directories().contains(fileInfo.absolutePath()))
        fileWatcher_->addPath(fileInfo.absolutePath());

    tagged_files_proxy_->sort(0);
    emit fileAddedExternally(absolutePath);
}
