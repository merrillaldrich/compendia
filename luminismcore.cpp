#include "luminismcore.h"
#include <QDirIterator>
#include <QDebug>

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
    QVector<std::tuple<QString, QString, QMap<QString, QString>, QImage>> batch;
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
        QImage img = std::get<3>(t);

        // Update the model using an icon based on the image
        applyBackfillMetadataToModel(fileName, path, exifMap, img);
        emit iconUpdated();
    }
}

/*! \brief Applies a generated thumbnail and EXIF data to the matching model item.
 *
 * \param fileName             The file name used to locate the model item.
 * \param absoluteFilePathName Full path used to disambiguate duplicate file names.
 * \param exifMap              EXIF key-value data to store on the TaggedFile.
 * \param image                Scaled thumbnail image to use as the item icon.
 */
void LuminismCore::applyBackfillMetadataToModel(const QString &fileName,
                                                const QString &absoluteFilePathName,
                                                const QMap<QString, QString> exifMap,
                                                const QImage &image)
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

        int size = 100;
        // Create a transparent square pixmap
        QPixmap square(size, size);
        square.fill(Qt::transparent);

        // Center the scaled image in the square
        QPainter painter(&square);
        int x = (size - image.width()) / 2;
        int y = (size - image.height()) / 2;
        painter.drawImage(x, y, image);
        painter.end();

        item->setIcon(square);

        // NOTE: EXIF uses colons in BOTH the date and the time, not dashes
        QString captureDateString = exifMap["DateTime"];
        QDateTime captureDateTime = QDateTime::fromString(captureDateString, "yyyy:MM:dd HH:mm:ss");
        tf->imageCaptureDateTime = captureDateTime;
        tf->setExifMap(exifMap);

    } else {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
    }
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
    QDirIterator it(root_directory_, QStringList() << "*.jpg" << "*.JPG" << "*.heic" << "*.HEIC" << "*.png" << "*.PNG", QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile f(it.next());
        QFileInfo fileInfo(f.fileName());

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

    if (tagsJson.contains("tags")) {
        // New format: tags and exif are under separate keys
        tagSets = parseTagJson(tagsJson["tags"].toObject());
        QJsonObject exifObj = tagsJson["exif"].toObject();
        for (auto it = exifObj.begin(); it != exifObj.end(); ++it) {
            exifMap.insert(it.key(), it.value().toString());
        }
    } else {
        // Old format: top-level object is the tags map
        tagSets = parseTagJson(tagsJson);
    }

    addFile(fileInfo, tagSets, exifMap);
}

/*! \brief Adds a file to the model with an explicit tag list and optional initial EXIF map.
 *
 * \param fileInfo    File-system info for the file to add.
 * \param tags        List of TagSet values describing tags to apply.
 * \param initialExif Optional pre-loaded EXIF key-value map.
 */
void LuminismCore::addFile(QFileInfo fileInfo, QList<TagSet> tags, QMap<QString, QString> initialExif){
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

    QStringList files;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        QStandardItem *item = tagged_files_->item(i);
        if (!item) continue;
        auto tf = item->data().value<TaggedFile*>();
        files << (tf->filePath + "/" + tf->fileName);
    }

    // run one async task per file explicitly, via lamba to avoid map overload ambiguity
    for (const QString &path : files) {
        // queue icon generation to thread pool
        // but place the results in a threadsafe vector instead of trying
        // to apply directly, so we can throttle this appropriately
        QtConcurrent::run([this, path]() {

            QImage img = IconGenerator::generateIcon(path);
            QMap<QString, QString> exifMap = ExifParser::getExifMap(path);

            QString fileName = QFileInfo(path).fileName();

            // store result (thread-safe)
            {
                QMutexLocker lock(&resultsMutex_);
                results_.emplace_back(fileName, path, exifMap, std::move(img));
            }
            // ensure timer is running on GUI thread to flush results
            QMetaObject::invokeMethod(this, "ensureUiFlushTimerRunning", Qt::QueuedConnection);
        });
    }

    // start a timer that will apply a few icons per tick
    connect(&uiFlushTimer_, &QTimer::timeout, this, &LuminismCore::flushIconGeneratorQueue);
    uiFlushTimer_.setInterval(50);
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

/*! Retrieve a tag from the library by name.
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

/*! Retrieve a tag family from the library by name.
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

/*! Create a tag in the library using just its family name and tag name as strings
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

/*! Add a tag to the library if it doesn't exist
 *
 * \param tag The Tag pointer to insert.
 */
void LuminismCore::addLibraryTag(Tag* tag){
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

/*! \brief Parses a JSON object of tag-family to tag-name arrays into a TagSet list.
 *
 * \param tagsJson A JSON object mapping family name strings to arrays of tag name strings.
 * \return A list of TagSet values parsed from the JSON.
 */
QList<TagSet> LuminismCore::parseTagJson(QJsonObject tagsJson){
    QList<TagSet> tagSets;

    for (auto it = tagsJson.begin(); it != tagsJson.end(); ++it) {
        QJsonArray array = it.value().toArray();
        for (const QJsonValue &value : array){
            tagSets.append(TagSet(it.key(),value.toString()));
        }
    }

    return tagSets;
}
