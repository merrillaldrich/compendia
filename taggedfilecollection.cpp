#include "taggedfilecollection.h"
#include <QDebug>

TaggedFileCollection::TaggedFileCollection(QObject *parent)
    : QObject{parent}{
    tag_families_ = new QSet<TagFamily*>();
    tags_ = new QSet<Tag*>();

    //tagged_file_collection_ = new QList<TaggedFile*>();
    tagged_files_ = new QStandardItemModel(this);
    tagged_files_proxy_ = new FilterProxyModel(this);
    tagged_files_proxy_->setSourceModel(tagged_files_);

    //connect(this, &TaggedFileCollection::iconReady, this, &TaggedFileCollection::on_IconReady);
}

QStandardItemModel* TaggedFileCollection::getItemModel(){
    return tagged_files_;
}

FilterProxyModel* TaggedFileCollection::getItemModelProxy(){
    return tagged_files_proxy_;
}

bool TaggedFileCollection::containsFiles(){
    return ( tagged_files_->rowCount() > 0 );
}

QSet<Tag*>* TaggedFileCollection::getLibraryTags(){
    return tags_;
}

QSet<Tag*>* TaggedFileCollection::getAssignedTags(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    QSet<Tag*>* distinctTags = new QSet<Tag*>();

    for (int i = 0; i < tagged_files_->rowCount(); ++i){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        QSet<Tag*> &tgs = *itemAsTaggedFile->tags;
        distinctTags->unite(tgs);
    }
    return distinctTags;
}

QSet<Tag*>* TaggedFileCollection::getAssignedTags_FilteredFiles(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a visible file
    QSet<Tag*>* distinctTags = new QSet<Tag*>();

    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){

        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = getItemModelProxy()->mapToSource(proxyIndex);

        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

        QSet<Tag*> &tgs = *itemAsTaggedFile->tags;
        distinctTags->unite(tgs);
    }
    return distinctTags;
}

QSet<Tag*>* TaggedFileCollection::getFilterTags(){
    // Loop over files and make a distinct list of all tags that
    // are assigned to a file
    return tagged_files_proxy_->getFilterTags();
}

/*! Retrieve a tag from the library by name.
 *
 * \param tagFamilyName
 * \param tagName
 */
Tag* TaggedFileCollection::getTag(QString tagFamilyName, QString tagName){
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
TagFamily* TaggedFileCollection::getTagFamily(QString tagFamilyName){
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

void TaggedFileCollection::applyTag(Tag* tag){
    // Apply tag to all the files in the filtered file list
    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){
        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = getItemModelProxy()->mapToSource(proxyIndex);
        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        itemAsTaggedFile->tags->insert(tag);
    }
}

void TaggedFileCollection::applyTag(TaggedFile* f, TagSet t){
    Tag* tag = getTag(t.tagFamilyName, t.tagName);
    f->tags->insert(tag);
}

void TaggedFileCollection::unapplyTag(Tag* tag){
    // Remove tag from all the files in the filtered file list
    for (int i = 0; i < tagged_files_proxy_->rowCount(); ++i){
        QModelIndex proxyIndex = tagged_files_proxy_->index(i, 0);

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = getItemModelProxy()->mapToSource(proxyIndex);
        QVariant var = sourceIndex.data(Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        itemAsTaggedFile->tags->remove(tag);
    }
}

void TaggedFileCollection::unapplyTag(TaggedFile* file, Tag* tag){
    // Locate the file and remove the indicated tag from it
    for( int i = 0; i < tagged_files_->rowCount(); ++i ){
        QStandardItem* item = tagged_files_->item(i);
        QVariant var = item->data();
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();
        if(itemAsTaggedFile == file)
            itemAsTaggedFile->tags->remove(tag);
    }
}

/*! Create a tag in the library using just its family name and tag name as strings
 *
 *  \param tagFamilyName
 *  \param tagName
 */
Tag* TaggedFileCollection::addLibraryTag(QString tagFamilyName, QString tagName){

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
void TaggedFileCollection::addLibraryTag(Tag* tag){
    tags_->insert(tag);
}


TagFamily* TaggedFileCollection::addLibraryTagFamily(QString tagFamilyName){

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

void TaggedFileCollection::addLibraryTagFamily(TagFamily* tagFamily){
    if(! tag_families_->contains(tagFamily)){
        tag_families_->insert(tagFamily);
    }
}

QList<TagSet> TaggedFileCollection::parseTagJson(QJsonObject tagsJson){
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

void TaggedFileCollection::addFile(QFileInfo fileInfo){
    QList<TagSet> tagSets;
    addFile(fileInfo, tagSets);
}

void TaggedFileCollection::addFile(QFileInfo fileInfo, QJsonObject tagsJson){
    QList<TagSet> tagSets = parseTagJson(tagsJson);
    addFile(fileInfo, tagSets);
}

void TaggedFileCollection::addFile(QFileInfo fileInfo, QList<TagSet> tags){
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
    TaggedFile *tf = new TaggedFile(fileInfo, newOrExistingTags);
    // Standard items have just an icon and text

    // Make an icon moved to run async
    QPixmap squarePixmap = default_icon_;
    QStandardItem *i = new QStandardItem(QIcon(squarePixmap), tf->fileName);
    // In order to store the custom object box it as a variant and store in item.setdata()
    i->setData(QVariant::fromValue(tf));
    tagged_files_->appendRow(i);
}

void TaggedFileCollection::populateIcons(){

    QStringList files;
    for (int i = 0; i < tagged_files_->rowCount(); ++i) {
        QStandardItem *item = tagged_files_->item(i);
        if (!item) continue;
        auto tf = item->data().value<TaggedFile*>();
        files << (tf->filePath + "/" + tf->fileName);
    }

    // run one async task per file explicitly, to avoid map overload ambiguity
    for (const QString &path : files) {
        // queue icon generation to thread pool
        // but place the results in a threadsafe vector instead of trying
        // to apply directly, so we can throttle this appropriately
        QtConcurrent::run([this, path]() {
            QImage img = IconGenerator::generateIcon(path);
            QString fileName = QFileInfo(path).fileName();

            // store result (thread-safe)
            {
                QMutexLocker lock(&resultsMutex_);
                results_.emplace_back(fileName, path, std::move(img));
            }
            // ensure timer is running on GUI thread to flush results
            QMetaObject::invokeMethod(this, "ensureUiFlushTimerRunning", Qt::QueuedConnection);
        });
    }

    // start a timer that will apply a few icons per tick
    connect(&uiFlushTimer_, &QTimer::timeout, this, &TaggedFileCollection::flushIconGeneratorQueue);
    uiFlushTimer_.setInterval(50);
}

void TaggedFileCollection::ensureUiFlushTimerRunning(){
    if (!uiFlushTimer_.isActive()) uiFlushTimer_.start();
}

void TaggedFileCollection::flushIconGeneratorQueue(){
    const int maxPerTick = 8;
    QVector<std::tuple<QString, QString, QImage>> batch;
    {
        QMutexLocker lock(&resultsMutex_);
        int take = qMin(maxPerTick, results_.size());
        for (int i = 0; i < take; ++i) batch.append(std::move(results_.takeFirst()));
        if (results_.isEmpty()) uiFlushTimer_.stop();
    }

    for (auto &t : batch) {
        const QString &fileName = std::get<0>(t);
        const QString &path = std::get<1>(t);
        QImage img = std::get<2>(t);

        // Change QImage to QPixmap while preserving aspect ratio
        QPixmap pix = QPixmap::fromImage(img).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // update your model/view safely (small number per tick keeps UI responsive)
        applyIconToModel(fileName, path, pix);
    }
}

void TaggedFileCollection::applyIconToModel(const QString &fileName, const QString &absoluteFilePathName, const QPixmap &pixmap)
{
    // There could be files in different folders having the same name, but to make things quick
    // we find all files with a matching name in the model, and then zero in on the specific one
    // using the full path and name. Generally expect the names will be unique most of the time.

    auto matches = tagged_files_->findItems(fileName);
    if (matches.isEmpty()) {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
        return;
    }

    QStandardItem* item = nullptr;
    for (int i = 0; i < matches.count(); ++i){
        QStandardItem* currentItem = matches[i];
        QVariant var = currentItem->data(Qt::UserRole + 1);
        TaggedFile* tf = var.value<TaggedFile*>();

        if((tf->filePath + "/" + tf->fileName) == absoluteFilePathName)
            item = currentItem;
    }

    if( item != nullptr){
        item->setIcon(pixmap);
    } else {
        qDebug() << "Could not locate " + absoluteFilePathName + " to set icon";
    }
}

void TaggedFileCollection::renameFamily(QString oldName, QString newName){
    for(TagFamily *fam : *tag_families_) {
        if (fam->getName() == oldName){
            fam->setName(newName);
        }
    }
}

void TaggedFileCollection::setFileNameFilter(QString filterText){
    tagged_files_proxy_->setNameFilter(filterText);
}

void TaggedFileCollection::addTagFilter(Tag* tag){
    tagged_files_proxy_->addTagFilter(tag);
}

void TaggedFileCollection::removeTagFilter(Tag* tag){
    tagged_files_proxy_->removeTagFilter(tag);
}
