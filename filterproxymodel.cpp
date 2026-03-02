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

    if (folder_filter_ == "") {
        folderResult = true;
    }
    else {
        // Append a delimiter to the path so that a filter like "italy/" matches the
        // final segment "/pictures/vacation/italy" without any special-casing.
        // Mid-path delimiters in the filter ("pictures/", "vacation/") continue to
        // match normally since they are already present in the path.
        folderResult = (folderName + QLatin1Char('/')).contains(folder_filter_, Qt::CaseInsensitive);
    }

    bool tagResult = false;

    if (tags_.isEmpty()) {
        tagResult = true;
    } else {
        const QSet<Tag*> &fileTags = *itemAsTaggedFile->tags();
        if (tagFilterMode_ == AnyTag) {
            tagResult = tags_.intersects(fileTags);
        } else {
            // AllTags: every filter tag must be present on the file
            tagResult = true;
            for (Tag* t : tags_) {
                if (!fileTags.contains(t)) {
                    tagResult = false;
                    break;
                }
            }
        }
    }

    bool dateResult = false;
    if (!creation_date_.isValid()) {
        dateResult = true;
    } else {
        // Prefer EXIF capture date; fall back to filename-inferred date, then filesystem creation date
        QDate effectiveDate = itemAsTaggedFile->imageCaptureDateTime.isValid()
            ? itemAsTaggedFile->imageCaptureDateTime.date()
            : itemAsTaggedFile->fileNameInferredDate.isValid()
                ? itemAsTaggedFile->fileNameInferredDate.date()
                : itemAsTaggedFile->fileCreationDateTime.date();
        dateResult = effectiveDate == creation_date_;
    }

    return nameResult && folderResult && tagResult && dateResult;
}

/*! \brief Sets the filename substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against file names.
 */
void FilterProxyModel::setNameFilter(QString filterText){
    name_filter_ = filterText;
    invalidateFilter();
}

/*! \brief Sets the folder path substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against folder paths.
 */
void FilterProxyModel::setFolderFilter(QString filterText){
    folder_filter_ = filterText;
    invalidateFilter();
}

/*! \brief Adds a tag to the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to add to the filter.
 */
void FilterProxyModel::addTagFilter(Tag* tag){
    tags_.insert(tag);
    invalidateFilter();
}

/*! \brief Removes a tag from the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to remove from the filter.
 */
void FilterProxyModel::removeTagFilter(Tag* tag){
    tags_.remove(tag);
    invalidateFilter();
}

/*! \brief Returns a pointer to the set of tags currently used for filtering.
 *
 * \return Pointer to the internal active-filter tag set.
 */
QSet<Tag*>* FilterProxyModel::getFilterTags(){
    return &tags_;
}

/*! \brief Sets the creation-date filter and re-applies the filter.
 *
 * \param creationDate The date to filter by; an invalid QDate clears the filter.
 */
void FilterProxyModel::setFilterCreationDate(QDate creationDate) {
    creation_date_ = creationDate;
    invalidateFilter();
}

/*! \brief Sets the tag-filter mode and re-applies the filter.
 *
 * \param mode AnyTag (OR) or AllTags (AND).
 */
void FilterProxyModel::setTagFilterMode(TagFilterMode mode) {
    tagFilterMode_ = mode;
    invalidateFilter();
}
