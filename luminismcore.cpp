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
