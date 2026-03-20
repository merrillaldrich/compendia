#include "filterproxymodel.h"

/*! \brief Constructs the proxy model and sets up the parent.
 *
 * \param parent Optional Qt parent object.
 */
FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

/*! \brief Tests \a tf against all base filters (name, folder, tag, date, rating).
 *
 * Shared implementation used by both filterAcceptsRow() and passesNonIsolationFilters().
 * Does not consider the isolation set.
 *
 * \param tf The TaggedFile to evaluate.
 * \return True if \a tf passes all base filters.
 */
bool FilterProxyModel::passesBaseFilters(TaggedFile* tf) const
{
    bool nameResult = name_filter_.isEmpty()
        || tf->fileName.contains(name_filter_, Qt::CaseInsensitive);

    // Append a delimiter to the path so that a filter like "italy/" matches the
    // final segment "/pictures/vacation/italy" without any special-casing.
    bool folderResult = folder_filter_.isEmpty()
        || (tf->filePath + QLatin1Char('/')).contains(folder_filter_, Qt::CaseInsensitive);

    bool tagResult = false;
    if (tags_.isEmpty()) {
        tagResult = true;
    } else {
        const QSet<Tag*> &fileTags = *tf->tags();
        if (tagFilterMode_ == AnyTag) {
            tagResult = tags_.intersects(fileTags);
        } else {
            // AllTags: every filter tag must be present on the file
            tagResult = true;
            for (Tag* t : tags_) {
                if (!fileTags.contains(t)) { tagResult = false; break; }
            }
        }
    }

    bool dateResult = !creation_date_.isValid()
        || tf->effectiveDate() == creation_date_;

    bool ratingResult = true;
    if (rating_filter_.has_value()) {
        const std::optional<int> fileRating = tf->rating();
        if (!fileRating.has_value()) {
            ratingResult = false;
        } else {
            const int fv = fileRating.value();
            const int rv = rating_filter_.value();
            switch (rating_filter_mode_) {
            case LessOrEqual:    ratingResult = (fv <= rv); break;
            case Exactly:        ratingResult = (fv == rv); break;
            case GreaterOrEqual: ratingResult = (fv >= rv); break;
            }
        }
    }

    return nameResult && folderResult && tagResult && dateResult && ratingResult;
}

/*! \brief Overrides the Qt base-class row-acceptance test to apply all active filters.
 *
 * \param sourceRow    The row index in the source model to test.
 * \param sourceParent The parent index in the source model.
 * \return True if the row passes all active filters.
 */
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex i = sourceModel()->index(sourceRow, 0, sourceParent);
    TaggedFile* tf = i.data(Qt::UserRole + 1).value<TaggedFile*>();

    // Isolation filter — evaluated first so non-isolated files are rejected cheaply.
    if (!isolation_set_.isEmpty() && !isolation_set_.contains(tf))
        return false;

    return passesBaseFilters(tf);
}

/*! \brief Sets the filename substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against file names.
 */
void FilterProxyModel::setNameFilter(QString filterText){
    beginFilterChange();
    name_filter_ = filterText;
    endFilterChange();
}

/*! \brief Sets the folder path substring filter and re-applies the filter.
 *
 * \param filterText The substring to match against folder paths.
 */
void FilterProxyModel::setFolderFilter(QString filterText){
    beginFilterChange();
    folder_filter_ = filterText;
    endFilterChange();
}

/*! \brief Adds a tag to the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to add to the filter.
 */
void FilterProxyModel::addTagFilter(Tag* tag){
    beginFilterChange();
    tags_.insert(tag);
    endFilterChange();
}

/*! \brief Removes a tag from the active tag filter set and re-applies the filter.
 *
 * \param tag The Tag pointer to remove from the filter.
 */
void FilterProxyModel::removeTagFilter(Tag* tag){
    beginFilterChange();
    tags_.remove(tag);
    endFilterChange();
}

/*! \brief Clears all active tag filters and re-applies the filter. */
void FilterProxyModel::clearTagFilters()
{
    beginFilterChange();
    tags_.clear();
    endFilterChange();
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
    beginFilterChange();
    creation_date_ = creationDate;
    endFilterChange();
}

/*! \brief Sets the tag-filter mode and re-applies the filter.
 *
 * \param mode AnyTag (OR) or AllTags (AND).
 */
void FilterProxyModel::setTagFilterMode(TagFilterMode mode) {
    beginFilterChange();
    tagFilterMode_ = mode;
    endFilterChange();
}

/*! \brief Sets the rating filter and re-applies the filter.
 *
 * \param rating The rating to filter for [1,5], or std::nullopt to disable.
 */
void FilterProxyModel::setRatingFilter(std::optional<int> rating) {
    beginFilterChange();
    rating_filter_ = rating;
    endFilterChange();
}

/*! \brief Sets the comparison mode used when evaluating the rating filter and re-applies the filter.
 *
 * \param mode LessOrEqual, Exactly, or GreaterOrEqual.
 */
void FilterProxyModel::setRatingFilterMode(RatingFilterMode mode) {
    beginFilterChange();
    rating_filter_mode_ = mode;
    endFilterChange();
}

void FilterProxyModel::setIsolationSet(const QSet<TaggedFile*> &files) {
    beginFilterChange();
    isolation_set_ = files;
    endFilterChange();
}

/*! \brief Clears the isolation set so all files are eligible again.
 */
void FilterProxyModel::clearIsolationSet() {
    beginFilterChange();
    isolation_set_.clear();
    endFilterChange();
}

/*! \brief Returns true when an isolation set is active.
 *
 * \return True if isolation_set_ is non-empty.
 */
bool FilterProxyModel::isIsolated() const {
    return !isolation_set_.isEmpty();
}

/*! \brief Returns the number of files in the current isolation set.
 *
 * \return Size of isolation_set_, or 0 when not isolated.
 */
int FilterProxyModel::isolationSetSize() const {
    return isolation_set_.size();
}

/*! \brief Tests \a tf against all active filters except the isolation set.
 *
 * Evaluates the name, folder, tag, date, and rating filters in the same
 * way as filterAcceptsRow(), but deliberately ignores isolation_set_.
 * Use this to detect files that would be hidden by the current filters
 * even if the isolation set were replaced.
 *
 * \param tf The TaggedFile to evaluate.
 * \return True if \a tf passes all non-isolation filters.
 */
bool FilterProxyModel::passesNonIsolationFilters(TaggedFile* tf) const
{
    return passesBaseFilters(tf);
}
