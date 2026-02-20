#include "luminismcore.h"
#include <QDirIterator>
#include <QDebug>

LuminismCore::LuminismCore(QObject *parent)
    : QObject{parent}{

    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();

    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);
    tagged_files_proxy_->sort(0);

}

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
        //QDateTime::fromString("2026-01-06 14:35:00", "yyyy-MM-dd HH:mm:ss");

        // NOTE: EXIF uses colons in BOTH the date and the time, not dashes
        QString captureDateString = exifMap["DateTime"];
        QDateTime captureDateTime = QDateTime::fromString(captureDateString, "yyyy:MM:dd HH:mm:ss");
        tf->imageCaptureDateTime = captureDateTime;
        tf->setExifMap(exifMap);

    } else {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
    }
}

void LuminismCore::setRootDirectory(QString path){
    root_directory_ = path;
    loadRootDirectory();
}

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

bool LuminismCore::containsFiles(){
    return ( tagged_files_->rowCount() > 0 );
}
void LuminismCore::addFile(QFileInfo fileInfo){
    QList<TagSet> tagSets;
    addFile(fileInfo, tagSets);
}

void LuminismCore::addFile(QFileInfo fileInfo, QJsonObject tagsJson){
    QList<TagSet> tagSets = parseTagJson(tagsJson);
    addFile(fileInfo, tagSets);
}

void LuminismCore::addFile(QFileInfo fileInfo, QList<TagSet> tags){
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
    // Standard items have just an icon and text

    // Make an icon moved to run async
    QPixmap squarePixmap = default_icon_;
    QStandardItem *i = new QStandardItem(QIcon(squarePixmap), tf->fileName);
    // In order to store the custom object box it as a variant and store in item.setdata()
    i->setData(QVariant::fromValue(tf));
    tagged_files_->appendRow(i);
}

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

void LuminismCore::ensureUiFlushTimerRunning(){
    if (!uiFlushTimer_.isActive()) uiFlushTimer_.start();
}

void LuminismCore::writeFileMetadata(){

    for (int row = 0; row < tagged_files_->rowCount(); ++row) {
        QVariant fi = tagged_files_->item(row)->data();
        TaggedFile* itemAsTaggedFile = fi.value<TaggedFile*>();

        bool shouldWrite = itemAsTaggedFile->dirtyFlag();
        if (!shouldWrite) {
            for (Tag* tag : *itemAsTaggedFile->tags()) {
                if (tag->dirtyFlag() || tag->tagFamily->dirtyFlag()) {
                    shouldWrite = true;
                    break;
                }
            }
        }
        if (!shouldWrite)
            continue;

        QString origFile = itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName;
        QFileInfo fileInfo(origFile);
        QString metaFilePath = itemAsTaggedFile->filePath + "/" +fileInfo.baseName() + ".json";


        //qDebug() << "Write to " << metaFilePath;
        //qDebug() << itemAsTaggedFile->TaggedFileJSON();

        QFile metaFile(metaFilePath);

        if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Could not open file for writing:" << metaFile.errorString();
            //qCritical() << "Could not open file for writing:" << metaFile.errorString();
            //return 1;
        }

        QTextStream out(&metaFile);
        out << itemAsTaggedFile->TaggedFileJSON();
        metaFile.close();
        itemAsTaggedFile->clearDirtyFlag();
        emit metadataSaved();
    }

    for (Tag* tag : *tags_)
        tag->clearDirtyFlag();
    for (TagFamily* family : *tag_families_)
        family->clearDirtyFlag();
}

QSet<Tag*>* LuminismCore::getLibraryTags(){
    return tags_;
}

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

QSet<Tag*>* LuminismCore::getFilterTags(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    return tagged_files_proxy_->getFilterTags();
}

/*! Retrieve a tag from the library by name.
 *
 * \param tagFamilyName
 * \param tagName
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
 * \param tagFamilyName
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
 *  \param tagFamilyName
 *  \param tagName
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
 * \param tag
 */
void LuminismCore::addLibraryTag(Tag* tag){
    tags_->insert(tag);
}


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

void LuminismCore::addLibraryTagFamily(TagFamily* tagFamily){
    if(! tag_families_->contains(tagFamily)){
        tag_families_->insert(tagFamily);
    }
}

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

void LuminismCore::applyTag(TaggedFile* f, TagSet t){
    Tag* tag = getTag(t.tagFamilyName, t.tagName);
    f->addTag(tag);
}

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

QStandardItemModel* LuminismCore::getItemModel(){
    return tagged_files_;
}

FilterProxyModel* LuminismCore::getItemModelProxy(){
    return tagged_files_proxy_;
}

void LuminismCore::setFileNameFilter(QString filterText){
    tagged_files_proxy_->setNameFilter(filterText);
}

void LuminismCore::setFolderFilter(QString filterText){
    tagged_files_proxy_->setFolderFilter(filterText);
}

void LuminismCore::addTagFilter(Tag* tag){
    tagged_files_proxy_->addTagFilter(tag);
}

void LuminismCore::removeTagFilter(Tag* tag){
    tagged_files_proxy_->removeTagFilter(tag);
}

QList<TagSet> LuminismCore::parseTagJson(QJsonObject tagsJson){
    QList<TagSet> tagSets;

    for (auto it = tagsJson.begin(); it != tagsJson.end(); ++it) {
        //qDebug() << "Key:" << it.key() << "Value:" << it.value();
        QJsonArray array = it.value().toArray();
        for (const QJsonValue &value : array){
            //qDebug() << "Tag:" << value.toString();
            tagSets.append(TagSet(it.key(),value.toString()));
        }
    }

    return tagSets;
}
