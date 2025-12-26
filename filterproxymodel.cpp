#include "filterproxymodel.h"

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

void FilterProxyModel::setNameFilter(QString filterText){
    beginFilterChange();
    name_filter_ = filterText;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex i = sourceModel()->index(sourceRow, 0, sourceParent);
    QVariant val = i.data(Qt::UserRole + 1);
    TaggedFile* itemAsTaggedFile = val.value<TaggedFile*>();
    QString fileName = itemAsTaggedFile->fileName;

    bool nameResult = false;

    if (name_filter_ == ""){
        nameResult = true;
    }
    else {
        nameResult = fileName.contains(name_filter_, Qt::CaseInsensitive);
    }

    bool tagResult = false;

    if (tags_.isEmpty()){
        tagResult = true;
    } else {
        const QSet<Tag*> tagsconst = *itemAsTaggedFile->tags;
        tagResult = tags_.intersects(tagsconst);
    }

    return nameResult && tagResult;
}

void FilterProxyModel::addTagFilter(Tag* tag){
    beginFilterChange();
    tags_.insert(tag);
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void FilterProxyModel::removeTagFilter(Tag* tag){
    beginFilterChange();
    tags_.remove(tag);
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}
