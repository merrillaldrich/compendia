#include "filterproxymodel.h"

/*! \brief Constructs the proxy model and sets up the parent.
 *
 * \param parent Optional Qt parent object.
 */
FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

/*! \brief Overrides the Qt base-class row-acceptance test to apply all active filters.
 *
 * \param sourceRow    The row index in the source model to test.
 * \param sourceParent The parent index in the source model.
 * \return True if the row passes all active filters.
 */
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

    QString folderName = itemAsTaggedFile->filePath;
    bool folderResult = false;

    if (folder_filter_ == ""){
        folderResult = true;
    }
    else {
        folderResult = folderName.contains(folder_filter_, Qt::CaseInsensitive);
    }

    bool tagResult = false;

    if (tags_.isEmpty()){
        tagResult = true;
    } else {
        const QSet<Tag*> tagsconst = *itemAsTaggedFile->tags();
        tagResult = tags_.intersects(tagsconst);
    }

    return nameResult && folderResult && tagResult;
}

/*! \brief Sets the filename substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against file names.
 */
void FilterProxyModel::setNameFilter(QString filterText){
    beginFilterChange();
    name_filter_ = filterText;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/*! \brief Sets the folder path substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against folder paths.
 */
void FilterProxyModel::setFolderFilter(QString filterText){
    beginFilterChange();
    folder_filter_ = filterText;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/*! \brief Adds a tag to the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to add to the filter.
 */
void FilterProxyModel::addTagFilter(Tag* tag){
    beginFilterChange();
    tags_.insert(tag);
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/*! \brief Removes a tag from the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to remove from the filter.
 */
void FilterProxyModel::removeTagFilter(Tag* tag){
    beginFilterChange();
    tags_.remove(tag);
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/*! \brief Returns a pointer to the set of tags currently used for filtering.
 *
 * \return Pointer to the internal active-filter tag set.
 */
QSet<Tag*>* FilterProxyModel::getFilterTags(){
    return &tags_;
}
