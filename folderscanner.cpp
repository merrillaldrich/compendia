#include "folderscanner.h"
#include "constants.h"

#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>

FolderScanner::FolderScanner(QObject *parent)
    : QObject(parent)
{}

void FolderScanner::cancel()
{
    cancelled_.storeRelaxed(1);
}

void FolderScanner::scan(const QString &rootPath)
{
    cancelled_.storeRelaxed(0);

    QDirIterator it(rootPath, QStringList()
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

    QList<ScanItem> batch;
    batch.reserve(300);

    while (it.hasNext()) {
        if (cancelled_.loadRelaxed())
            break;

        QFile f(it.next());
        QFileInfo fileInfo(f.fileName());

        if (fileInfo.fileName().startsWith(".trashed", Qt::CaseInsensitive))
            continue;

        if (fileInfo.absolutePath().contains(QLatin1String(Luminism::CacheFolderName)))
            continue;

        ScanItem item;
        item.fileInfo = fileInfo;

        QString metaFileName = fileInfo.baseName() + ".json";
        QFileInfo metaFileInfo(fileInfo.absolutePath() + "/" + metaFileName);

        if (metaFileInfo.exists() && metaFileInfo.isFile()) {
            QFile metaFile(fileInfo.absolutePath() + "/" + metaFileName);
            if (metaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QJsonDocument d = QJsonDocument::fromJson(metaFile.readAll());
                metaFile.close();
                item.tagsJson = d.object();
                item.hasJson = true;
            }
        }

        batch.append(item);

        if (batch.size() >= 300) {
            emit batchReady(batch);
            batch.clear();
            batch.reserve(300);
        }
    }

    if (!batch.isEmpty())
        emit batchReady(batch);

    emit finished();
}
