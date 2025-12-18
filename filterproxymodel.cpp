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

if(name_filter_ == ""){
        return true;
    }
    else {
        bool result = fileName.contains(name_filter_, Qt::CaseInsensitive);
        return result;
    }
}
