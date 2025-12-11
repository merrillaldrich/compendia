#include "luminismcore.h"
#include <QDirIterator>
#include <QDebug>

LuminismCore::LuminismCore(QObject *parent)
    : QObject{parent}
{}

void LuminismCore::setRootDirectory(QString path){
    root_directory_ = path;
    loadRootDirectory();
}

void LuminismCore::loadRootDirectory(){

    // Pointing to a new folder means we have to clear any data already loaded:
    if ( tfc->containsFiles() ){
        delete tfc;
        tfc = new TaggedFileCollection();
    }

    QDirIterator it(root_directory_, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile f(it.next());
        QFileInfo fileInfo(f.fileName());

        // Dummy empty tag sets
        QList<TagSet> tags = QList<TagSet>();

        tfc->addFile(fileInfo.absolutePath(), fileInfo.fileName(), tags);
    }
}

void LuminismCore::writeFileMetadata(){

    QStandardItemModel* model = tfc->getItemModel();

    for (int row = 0; row < model->rowCount(); ++row) {
        QVariant fi = model->item(row)->data();
        TaggedFile* itemAsTaggedFile = fi.value<TaggedFile*>();

        qDebug() << itemAsTaggedFile->fileName;

        QList<Tag*>* tagList = itemAsTaggedFile->tagList;

        for ( int i = 0; i < tagList->count(); ++i){
            Tag* t = tagList->at(i);
            qDebug() << t->tagFamily->tagFamilyName << t->tagName;            
        }

        qDebug() << itemAsTaggedFile->TaggedFileJSON();
    }
}

Tag* LuminismCore::getTag(QString tagFamily, QString tag){
    return tfc->getTag(tagFamily, tag);
}

Tag* LuminismCore::addLibraryTag(QString tagFamily, QString tag){
    return tfc->addLibraryTag(tagFamily, tag);
}

TagFamily* LuminismCore::addLibraryTagFamily(QString tagFamily){
    return tfc->addLibraryTagFamily(tagFamily);
}

void LuminismCore::applyTag(Tag* droppedTag){
    tfc->applyTag(droppedTag);
}

QStandardItemModel* LuminismCore::getItemModel(){
    return tfc->getItemModel();
}
