#include "luminismcore.h"
#include <QDirIterator>
#include <QDebug>

LuminismCore::LuminismCore(QObject *parent)
    : QObject{parent}{
    tfc = new TaggedFileCollection(this);
}

void LuminismCore::setRootDirectory(QString path){
    root_directory_ = path;
    loadRootDirectory();
}

void LuminismCore::loadRootDirectory(){

    // Pointing to a new folder means we have to clear any data already loaded:
    if ( tfc->containsFiles() ){
        delete tfc;
        tfc = new TaggedFileCollection(this);
    }

    QDirIterator it(root_directory_, QStringList() << "*.jpg", QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
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

            tfc->addFile(fileInfo.absolutePath(), fileInfo.fileName(), tagsJson);

        } else {
            // Otherwise just add the file with no tags
            tfc->addFile(fileInfo.absolutePath(), fileInfo.fileName());
        }
    }
}

void LuminismCore::writeFileMetadata(){

    QStandardItemModel* model = tfc->getItemModel();

    for (int row = 0; row < model->rowCount(); ++row) {
        QVariant fi = model->item(row)->data();
        TaggedFile* itemAsTaggedFile = fi.value<TaggedFile*>();

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
    }
}

QList<Tag*>* LuminismCore::getLibraryTags(){
    return tfc->getLibraryTags();
}

QList<Tag*>* LuminismCore::getAssignedTags(){
    return tfc->getAssignedTags();
}

Tag* LuminismCore::getTag(QString tagFamilyName, QString tagName){
    return tfc->getTag(tagFamilyName, tagName);
}

TagFamily* LuminismCore::getTagFamily(QString tagFamilyName){
    return tfc->getTagFamily(tagFamilyName);
}

Tag* LuminismCore::addLibraryTag(QString tagFamily, QString tag){
    return tfc->addLibraryTag(tagFamily, tag);
}

void LuminismCore::addLibraryTag(Tag* tag){
    tfc->addLibraryTag(tag);
}

TagFamily* LuminismCore::addLibraryTagFamily(QString tagFamily){
    return tfc->addLibraryTagFamily(tagFamily);
}

void LuminismCore::addLibraryTagFamily(TagFamily* tagFamily){
    tfc->addLibraryTagFamily(tagFamily);
}

void LuminismCore::applyTag(Tag* droppedTag){
    tfc->applyTag(droppedTag);
}

void LuminismCore::applyTag(TaggedFile* file, TagSet tagSet){
    tfc->applyTag(file, tagSet);
}

QStandardItemModel* LuminismCore::getItemModel(){
    return tfc->getItemModel();
}

FilterProxyModel* LuminismCore::getItemModelProxy(){
    return tfc->getItemModelProxy();
}

void LuminismCore::setFileNameFilter(QString filterText){
    tfc->setFileNameFilter(filterText);
}
