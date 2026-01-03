#ifndef LUMINISMCORE_H
#define LUMINISMCORE_H

#include <QObject>
#include <QDir>
#include <QDebug>
#include <QStandardItemModel>
#include <QMutex>
#include <QTimer>
#include <QtConcurrent>
#include "tagset.h"
#include "filterproxymodel.h"
#include "icongenerator.h"

class LuminismCore : public QObject
{
    Q_OBJECT
private:

    QString root_directory_;

    QSet<TagFamily*>* tag_families_;
    QSet<Tag*>* tags_;
    QStandardItemModel *tagged_files_;
    FilterProxyModel *tagged_files_proxy_;

    QMutex resultsMutex_;
    QVector<std::tuple<QString, QString, QImage>> results_; // fileName, path, image
    QTimer uiFlushTimer_;

    QPixmap default_icon_ = QPixmap(":/resources/NoImagePreviewIcon.png");

    void flushIconGeneratorQueue();
    void applyIconToModel(const QString &fileName, const QString &absoluteFilePathName, const QImage &image);

public:

    explicit LuminismCore(QObject *parent = nullptr);

    void setRootDirectory(QString path);
    void loadRootDirectory();

    bool containsFiles();

    void addFile(QFileInfo fileInfo);
    void addFile(QFileInfo fileInfo, QJsonObject tagsJson);
    void addFile(QFileInfo fileInfo, QList<TagSet> tags);
    void populateIcons();


    void writeFileMetadata();

    QSet<Tag*>* getLibraryTags();
    QSet<Tag*>* getAssignedTags();
    QSet<Tag*>* getAssignedTags_FilteredFiles();
    QSet<Tag*>* getFilterTags();

    Tag* getTag(QString tagFamilyName, QString tagName);
    TagFamily* getTagFamily(QString tagFamilyName);
    Tag* addLibraryTag(QString tagFamily, QString tagName);
    void addLibraryTag(Tag* tag);
    TagFamily* addLibraryTagFamily(QString tagFamilyName);
    void addLibraryTagFamily(TagFamily* tagFamily);
    void applyTag(Tag* tag);
    void applyTag(TaggedFile* file, TagSet tagSet);
    void unapplyTag(TaggedFile* file, Tag* tag);
    void unapplyTag(Tag* tag);

    void setFileNameFilter(QString filterText);
    void setFolderFilter(QString filterText);

    void addTagFilter(Tag* tag);
    void removeTagFilter(Tag* tag);

    QStandardItemModel* getItemModel();
    FilterProxyModel* getItemModelProxy();

    QList<TagSet> parseTagJson(QJsonObject tagsJson);

public slots:

    void ensureUiFlushTimerRunning();

signals:
    void iconUpdated();
    void metadataSaved();
};

#endif // LUMINISMCORE_H
