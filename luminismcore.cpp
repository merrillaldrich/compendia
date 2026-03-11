#include "luminismcore.h"
#include "constants.h"
#include "perceptualhasher.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>
#include <QSet>

/*! \brief Constructs LuminismCore and initialises empty model and tag-library containers.
 *
 * \param parent Optional Qt parent object.
 */
LuminismCore::LuminismCore(QObject *parent)
    : QObject{parent}{

    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();

    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);
    tagged_files_proxy_->sort(0);

}

/*! \brief Moves up to a fixed number of pending icon results from the background queue into the model. */
void LuminismCore::flushIconGeneratorQueue(){
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

/*! \brief Applies generated thumbnails and EXIF data to the matching model item.
 *
 * \param fileName             The file name used to locate the model item.
 * \param absoluteFilePathName Full path used to disambiguate duplicate file names.
 * \param exifMap              EXIF key-value data to store on the TaggedFile.
 * \param images               Scaled thumbnail images (one per kIconSizes entry) used to
 *                             build a multi-size QIcon so Qt always downscales cleanly.
 */
void LuminismCore::applyBackfillMetadataToModel(const QString &fileName,
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

    if( item != nullptr){

        // Build a multi-size QIcon so Qt picks the best-fit size at every zoom level.
        // Each image is letterboxed into a square pixmap to keep consistent icon geometry.
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
        item->setIcon(icon);

        // NOTE: EXIF uses colons in BOTH the date and the time, not dashes
        QString captureDateString = exifMap["DateTime"];
        QDateTime captureDateTime = QDateTime::fromString(captureDateString, "yyyy:MM:dd HH:mm:ss");

        // Fallback for video container metadata (QMediaMetaData::Date → ISO 8601)
        if (!captureDateTime.isValid() && exifMap.contains("Date"))
            captureDateTime = QDateTime::fromString(exifMap["Date"], Qt::ISODate);

        tf->imageCaptureDateTime = captureDateTime;
        if (!exifMap.isEmpty())
            tf->setExifMap(exifMap);
        if (pHash != 0)
            tf->setPHash(pHash);

    } else {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
    }
}

/*! \brief Updates the in-memory icon for the file at \p absoluteFilePath using the provided images.
 *
 * \param absoluteFilePath Absolute path to the video file whose icon to replace.
 * \param images           Scaled thumbnail images (one per IconGenerator::kIconSizes entry).
 */
void LuminismCore::updateFileIcons(const QString &absoluteFilePath, const QVector<QImage> &images)
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
        candidate->setIcon(icon);
        emit iconUpdated();
        return;
    }

    qDebug() << "updateFileIcons: could not match path for" << absoluteFilePath;
}

/*! \brief Applies video container metadata to the TaggedFile for \p absoluteFilePath.
 *
 * \param absoluteFilePath Absolute path to the video file.
 * \param meta             Container metadata map produced by FrameGrabber.
 */
void LuminismCore::applyVideoMetadata(const QString &absoluteFilePath,
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
void LuminismCore::setRootDirectory(QString path){
    root_directory_ = path;
    loadRootDirectory();
}

/*! \brief Clears all existing data and reloads files from the current root directory. */
void LuminismCore::loadRootDirectory(){

    // Tear down any in-flight icon generation before destroying the model
    if (iconGenerator_) {
        iconGenerator_->disconnect();
        iconGenerator_->deleteLater();
        iconGenerator_ = nullptr;
    }
    uiFlushTimer_.stop();
    { QMutexLocker lock(&resultsMutex_); results_.clear(); }

    // Pointing to a new folder means we have to clear any data already loaded:
    delete tagged_files_proxy_;
    delete tagged_files_;
    delete tags_;
    delete tag_families_;

    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();
    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);

    tagged_files_proxy_->setSourceModel(tagged_files_);
    tagged_files_proxy_->sort(0);


    // Iterate over the entire root directory recursively and populate the model
    QDirIterator it(root_directory_, QStringList()
                        << "*.jpg" << "*.JPG"
                        << "*.heic" << "*.HEIC"
                        << "*.png" << "*.PNG"
                        << "*.mp4" << "*.MP4"
                        << "*.mov" << "*.MOV"
                        << "*.avi" << "*.AVI"
                        << "*.mkv" << "*.MKV"
                        << "*.wmv" << "*.WMV"
                        << "*.webm" << "*.WEBM"
                        << "*.m4v" << "*.M4V",
                    QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile f(it.next());
        QFileInfo fileInfo(f.fileName());

        // Skip files whose names start with ".trashed" — some phones use this
        // prefix to soft-delete files (acting as a recycle bin).
        if (fileInfo.fileName().startsWith(".trashed", Qt::CaseInsensitive))
            continue;

        // Skip files inside any cache folder to avoid caching the cache.
        if (fileInfo.absolutePath().contains(QLatin1String(Luminism::CacheFolderName)))
            continue;

        // Check to see if there's a sidecar file next to each image file
        QString metaFileName = fileInfo.baseName() + ".json";
        QFileInfo metaFileInfo(fileInfo.absolutePath() + "/" + metaFileName);

        if(metaFileInfo.exists() && metaFileInfo.isFile()){
            // If so then add the file using the tags in that sidecar file
            QString val;

            QFile metaFile;
            metaFile.setFileName(fileInfo.absolutePath() + "/" + metaFileName);
            metaFile.open(QIODevice::ReadOnly | QIODevice::Text);
            val = metaFile.readAll();
            metaFile.close();

            QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
            QJsonObject tagsJson = d.object();

            addFile(fileInfo, tagsJson);

        } else {
            // Otherwise just add the file with no tags
            addFile(fileInfo);
        }
    }
    // Queue populating icons & EXIF data on another thread
    backfillMetadata();
}

/*! \brief Returns true if the model currently contains at least one file.
 *
 * \return True when the model row count is greater than zero.
 */
bool LuminismCore::containsFiles(){
    return ( tagged_files_->rowCount() > 0 );
}

/*! \brief Returns summary statistics for all files in the source model.
 *
 * Files inside any .luminism_cache sub-directory are excluded.
 * \return A FolderStats value with image/video counts, folder count, and total size.
 */
LuminismCore::FolderStats LuminismCore::getFolderStats() const
{
    FolderStats stats;
    QSet<QString> folders;
    const int n = tagged_files_->rowCount();
    for (int i = 0; i < n; ++i) {
        const QVariant val = tagged_files_->item(i)->data(Qt::UserRole + 1);
        const TaggedFile *tf = val.value<TaggedFile*>();
        if (!tf) continue;
        if (tf->filePath.contains(QLatin1String(Luminism::CacheFolderName))) continue;
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
void LuminismCore::addFile(QFileInfo fileInfo){
    QList<TagSet> tagSets;
    addFile(fileInfo, tagSets);
}

/*! \brief Adds a file to the model, parsing tags and EXIF from a JSON object.
 *
 * \param fileInfo The file-system info for the file to add.
 * \param tagsJson A JSON object containing tag and optional EXIF data.
 */
void LuminismCore::addFile(QFileInfo fileInfo, QJsonObject tagsJson){
    QList<TagSet> tagSets;
    QMap<QString, QString> exifMap;
    quint64 pHash = 0;

    if (tagsJson.contains("tags")) {
        // New format: tags and exif are under separate keys
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
    } else {
        // Old format: top-level object is the tags map
        tagSets = parseTagJson(tagsJson);
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
void LuminismCore::addFile(QFileInfo fileInfo, QList<TagSet> tags, QMap<QString, QString> initialExif, quint64 pHash, std::optional<int> rating){
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
void LuminismCore::backfillMetadata(){

    // Fix pre-existing duplicate-connection bug by disconnecting first
    uiFlushTimer_.disconnect();
    connect(&uiFlushTimer_, &QTimer::timeout, this, &LuminismCore::flushIconGeneratorQueue);
    uiFlushTimer_.setInterval(50);

    QStringList allPaths;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        QStandardItem *item = tagged_files_->item(i);
        if (!item) continue;
        auto tf = item->data().value<TaggedFile*>();
        allPaths << (tf->filePath + "/" + tf->fileName);
    }
    std::sort(allPaths.begin(), allPaths.end(), [](const QString &a, const QString &b) {
        return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
    });

    iconGenerator_ = new IconGenerator(this);
    connect(iconGenerator_, &IconGenerator::fileReady,
            this, &LuminismCore::onIconReady);
    connect(iconGenerator_, &IconGenerator::batchFinished,
            this, &LuminismCore::onIconBatchFinished);
    iconGenerator_->processFiles(allPaths);
}

/*! \brief Receives a completed file result from IconGenerator and pushes it to the flush queue.
 *
 * \param absolutePath Absolute path to the source file.
 * \param exifMap      EXIF data (empty for videos).
 * \param images       Scaled thumbnail images.
 */
void LuminismCore::onIconReady(const QString &absolutePath,
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
void LuminismCore::onIconBatchFinished()
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

/*! \brief Starts the UI flush timer if it is not already running. */
void LuminismCore::ensureUiFlushTimerRunning(){
    if (!uiFlushTimer_.isActive()) uiFlushTimer_.start();
}

/*! \brief Returns true if any file in the model has unsaved changes.
 *
 * \return True when at least one TaggedFile has a dirty flag set.
 */
bool LuminismCore::hasUnsavedChanges()
{
    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        if (tagged_files_->item(row)->data().value<TaggedFile*>()->dirtyFlag())
            return true;
    }
    return false;
}

/*! \brief Writes JSON sidecar files for every file with unsaved changes. */
void LuminismCore::writeFileMetadata(){

    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        QVariant fi = tagged_files_->item(row)->data();
        TaggedFile* itemAsTaggedFile = fi.value<TaggedFile*>();

        if (itemAsTaggedFile->dirtyFlag()) {
            QString origFile = itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName;
            QFileInfo fileInfo(origFile);
            QString metaFilePath = itemAsTaggedFile->filePath + "/" + fileInfo.baseName() + ".json";

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

/*! \brief Writes JSON sidecar files for every visible (proxy-filtered) file with unsaved changes. */
void LuminismCore::writeVisibleFileMetadata()
{
    for (int row = 0; row < tagged_files_proxy_->rowCount(); ++row) {
        QModelIndex proxyIndex = tagged_files_proxy_->index(row, 0);
        QModelIndex srcIndex   = tagged_files_proxy_->mapToSource(proxyIndex);
        TaggedFile* itemAsTaggedFile = tagged_files_->itemFromIndex(srcIndex)->data(Qt::UserRole + 1).value<TaggedFile*>();

        if (itemAsTaggedFile->dirtyFlag()) {
            QString origFile = itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName;
            QFileInfo fileInfo(origFile);
            QString metaFilePath = itemAsTaggedFile->filePath + "/" + fileInfo.baseName() + ".json";

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
}

/*! \brief Clears the dirty flag on every file, tag, and tag family. */
void LuminismCore::clearAllDirtyFlags()
{
    for (int row = 0; row < tagged_files_->rowCount(); ++row)
        tagged_files_->item(row)->data().value<TaggedFile*>()->clearDirtyFlag();
    for (Tag* tag : *tags_)
        tag->clearDirtyFlag();
    for (TagFamily* family : *tag_families_)
        family->clearDirtyFlag();
}

/*! \brief Returns a pointer to the complete tag library.
 *
 * \return Pointer to the set of all known Tag objects.
 */
QSet<Tag*>* LuminismCore::getLibraryTags(){
    return tags_;
}

/*! \brief Returns a new set of tags that are assigned to at least one file.
 *
 * \return Heap-allocated set of assigned Tag pointers (caller takes ownership).
 */
QSet<Tag*>* LuminismCore::getAssignedTags(){
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
QSet<Tag*>* LuminismCore::getAssignedTags_FilteredFiles(){
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
QSet<Tag*>* LuminismCore::getFilterTags(){
    return tagged_files_proxy_->getFilterTags();
}

/*! \brief Retrieve a tag from the library by name.
 *
 * \param tagFamilyName The name of the tag family to search within.
 * \param tagName       The tag name to find.
 * \return Pointer to the matching Tag, or nullptr if not found.
 */
Tag* LuminismCore::getTag(QString tagFamilyName, QString tagName){
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
TagFamily* LuminismCore::getTagFamily(QString tagFamilyName){
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
Tag* LuminismCore::addLibraryTag(QString tagFamilyName, QString tagName){

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
    }
    return matchingTag;
}

/*! \brief Add a tag to the library if it doesn't exist.
 *
 * Transfers Qt ownership of \p tag to this LuminismCore instance so that the
 * tag is not accidentally destroyed when the UI widget that originally created
 * it (typically a TagFamilyWidget) is rebuilt and its children are deleted.
 *
 * \param tag The Tag pointer to insert.
 */
void LuminismCore::addLibraryTag(Tag* tag){
    tag->setParent(this);  // Take ownership: prevent widget destruction from deleting the tag
    tags_->insert(tag);
}

/*! \brief Creates or retrieves a tag family in the library by name.
 *
 * \param tagFamilyName The family name to create or find.
 * \return Pointer to the new or existing TagFamily.
 */
TagFamily* LuminismCore::addLibraryTagFamily(QString tagFamilyName){

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
    }
    return matchingFam;
}

/*! \brief Adds an existing TagFamily pointer to the library if not already present.
 *
 * \param tagFamily The TagFamily pointer to insert.
 */
void LuminismCore::addLibraryTagFamily(TagFamily* tagFamily){
    if(! tag_families_->contains(tagFamily)){
        tag_families_->insert(tagFamily);
    }
}

/*! \brief Applies a tag to all files currently visible in the filtered file list.
 *
 * \param tag The Tag to apply.
 */
void LuminismCore::applyTag(Tag* tag){
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
void LuminismCore::applyTag(TaggedFile* f, TagSet t){
    Tag* tag = getTag(t.tagFamilyName, t.tagName);
    f->addTag(tag);
}

/*! \brief Removes a tag from all files currently visible in the filtered file list.
 *
 * \param tag The Tag to remove.
 */
void LuminismCore::unapplyTag(Tag* tag){
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
int LuminismCore::countVisibleFilesWithTag(Tag* tag){
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
int LuminismCore::countAllFilesWithTag(Tag* tag){
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
void LuminismCore::deleteTagFromLibrary(Tag* tag){
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
void LuminismCore::unapplyTag(TaggedFile* file, Tag* tag){
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
QStandardItemModel* LuminismCore::getItemModel(){
    return tagged_files_;
}

/*! \brief Returns the FilterProxyModel used by the file list view.
 *
 * \return Pointer to the proxy model.
 */
FilterProxyModel* LuminismCore::getItemModelProxy(){
    return tagged_files_proxy_;
}

/*! \brief Sets the filename substring filter on the proxy model.
 *
 * \param filterText The substring to match against file names.
 */
void LuminismCore::setFileNameFilter(QString filterText){
    tagged_files_proxy_->setNameFilter(filterText);
}

/*! \brief Sets the folder path substring filter on the proxy model.
 *
 * \param filterText The substring to match against folder paths.
 */
void LuminismCore::setFolderFilter(QString filterText){
    tagged_files_proxy_->setFolderFilter(filterText);
}

/*! \brief Sets the creation-date filter on the proxy model.
 *
 * \param date The date to filter by; an invalid QDate clears the filter.
 */
void LuminismCore::setCreationDateFilter(QDate date) {
    tagged_files_proxy_->setFilterCreationDate(date);
}

/*! \brief Sets the rating filter on the proxy model.
 *
 * \param rating The rating to filter for [1,5], or std::nullopt to disable.
 */
void LuminismCore::setRatingFilter(std::optional<int> rating) {
    tagged_files_proxy_->setRatingFilter(rating);
}

/*! \brief Sets the comparison mode used when evaluating the rating filter.
 *
 * \param mode LessOrEqual, Exactly, or GreaterOrEqual.
 */
void LuminismCore::setRatingFilterMode(FilterProxyModel::RatingFilterMode mode) {
    tagged_files_proxy_->setRatingFilterMode(mode);
}

/*! \brief Returns a chronologically sorted list of unique effective dates across all loaded files.
 *
 * Each file's effective date is its EXIF capture date when present, falling back to the
 * filename-inferred date, then the filesystem creation date.
 * Used to populate the date-filter autocomplete.
 */
QList<QDate> LuminismCore::getFileDates() const
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
QList<QString> LuminismCore::getFileFolders() const
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
void LuminismCore::addTagFilter(Tag* tag){
    tagged_files_proxy_->addTagFilter(tag);
}

/*! \brief Removes a tag from the active tag filter set.
 *
 * \param tag The Tag to remove from the filter.
 */
void LuminismCore::removeTagFilter(Tag* tag){
    tagged_files_proxy_->removeTagFilter(tag);
}

/*! \brief Restricts the visible set to the given files; other filters still apply within the set.
 *
 * \param files The set of TaggedFile pointers to isolate.
 */
void LuminismCore::setIsolationSet(const QSet<TaggedFile*> &files) {
    tagged_files_proxy_->setIsolationSet(files);
}

/*! \brief Clears the isolation set so all files are eligible again.
 */
void LuminismCore::clearIsolationSet() {
    tagged_files_proxy_->clearIsolationSet();
}

/*! \brief Returns true when an isolation set is active.
 *
 * \return True if the proxy model has a non-empty isolation set.
 */
bool LuminismCore::isIsolated() const {
    return tagged_files_proxy_->isIsolated();
}

/*! \brief Returns the number of files in the current isolation set.
 *
 * \return Size of the isolation set, or 0 when not isolated.
 */
int LuminismCore::isolationSetSize() const {
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
QList<QList<TaggedFile*>> LuminismCore::findSimilarImages(int threshold)
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

/*! \brief Removes all auto-detected face tags from every file, the active
 *  filter, and the tag library. Cleans up now-empty tag families and emits
 *  tagLibraryChanged() once when done.
 *
 * \return Number of tags removed, or 0 if there were none.
 */
int LuminismCore::removeAutoDetectedFaceTags()
{
    // Collect auto tags from the library into a snapshot
    QList<Tag*> autoTags;
    for (Tag* t : *tags_) {
        if (t->getName().startsWith(Luminism::AutoFaceTagPrefix))
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
void LuminismCore::mergeTag(Tag* from, Tag* into)
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
void LuminismCore::mergeTagFamily(TagFamily* from, TagFamily* into)
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
        }
    }

    tag_families_->remove(from);
    from->deleteLater();
    emit tagLibraryChanged();
}

/*! \brief Parses a JSON object of tag-family to tag-name arrays into a TagSet list.
 *
 * \param tagsJson     A JSON object mapping family name strings to arrays of tag name strings.
 * \param tagRectsJson Optional JSON object mapping family → tag → [x,y,w,h] for bounding rects.
 * \return A list of TagSet values parsed from the JSON.
 */
QList<TagSet> LuminismCore::parseTagJson(QJsonObject tagsJson, QJsonObject tagRectsJson){
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
