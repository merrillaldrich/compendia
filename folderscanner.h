#ifndef FOLDERSCANNER_H
#define FOLDERSCANNER_H

#include <QObject>
#include <QFileInfo>
#include <QJsonObject>
#include <QAtomicInt>
#include <QList>
#include <QMap>
#include <QString>

/*! \brief Holds the per-file data produced during a background directory scan. */
struct ScanItem {
    QFileInfo   fileInfo;
    QJsonObject tagsJson;     ///< Parsed sidecar JSON; empty when hasJson is false.
    bool        hasJson = false;

    // Populated during scan when all icon cache files are valid.
    // hasCachedIcon = true means the file can skip IconGenerator entirely;
    // its icon will be loaded on demand by the LRU pool / delegate.
    bool                   hasCachedIcon = false;
    QMap<QString, QString> cachedExif;   ///< EXIF extracted from the source file; empty on cache miss.
};

Q_DECLARE_METATYPE(QList<ScanItem>)

/*! \brief Background worker that recursively scans a directory for media files.
 *
 * Intended to live on a dedicated QThread.  Emits batchReady() every ~300
 * files and finished() when the scan is complete or cancelled.
 */
class FolderScanner : public QObject {
    Q_OBJECT
public:
    explicit FolderScanner(QObject *parent = nullptr);

public slots:
    /*! \brief Scans \p rootPath recursively and emits file batches.
     *
     * \param rootPath Absolute path to the root media folder.
     */
    void scan(const QString &rootPath);

    /*! \brief Requests cancellation; the scan loop will stop on its next iteration. */
    void cancel();

signals:
    /*! \brief Emitted every ~300 files with the accumulated batch of scan results. */
    void batchReady(QList<ScanItem> batch);

    /*! \brief Emitted once when the scan finishes (normally or after cancellation). */
    void finished();

private:
    QAtomicInt cancelled_;
};

#endif // FOLDERSCANNER_H
