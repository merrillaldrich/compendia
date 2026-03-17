#ifndef FILTERPROXYMODEL_H
#define FILTERPROXYMODEL_H

#include <optional>
#include <QSortFilterProxyModel>
#include <QDate>
#include <QSet>
#include "taggedfile.h"

/*! \brief Proxy model that filters the tagged-file list by name, folder, and tags.
 *
 * Wraps a QStandardItemModel containing TaggedFile items and exposes only the
 * rows that satisfy the currently active name-substring, folder-substring, and
 * tag filters.  The tag filter can operate in AnyTag (OR) or AllTags (AND) mode.
 */
class FilterProxyModel : public QSortFilterProxyModel
{
public:
    /*! \brief Controls whether the tag filter requires any or all active tags. */
    enum TagFilterMode {
        AnyTag,  /*!< Show files that have at least one of the active filter tags (OR). */
        AllTags  /*!< Show files that have every active filter tag (AND). */
    };

    /*! \brief Controls how the rating filter value is compared against each file's rating. */
    enum RatingFilterMode {
        LessOrEqual,    /*!< Show files whose rating is ≤ the filter value. */
        Exactly,        /*!< Show files whose rating exactly matches the filter value. */
        GreaterOrEqual  /*!< Show files whose rating is ≥ the filter value. */
    };

private:
    QDate creation_date_;                           ///< Active creation-date filter; invalid means no filter.
    QSet<Tag*> tags_ = QSet<Tag*>();                ///< Set of tags currently active as filters.
    TagFilterMode tagFilterMode_ = AnyTag;          ///< Whether the tag filter uses OR or AND logic.
    QSet<TaggedFile*> isolation_set_; ///< When non-empty, only listed files can pass.
    std::optional<int> rating_filter_; ///< When set, only files matching the mode+value pass.
    RatingFilterMode rating_filter_mode_ = Exactly; ///< Comparison mode for the rating filter.

protected:
    /*! \brief Overrides the Qt base-class row-acceptance test to apply all active filters.
     *
     * \param sourceRow    The row index in the source model to test.
     * \param sourceParent The parent index in the source model.
     * \return True if the row passes all active filters.
     */
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

public:
    /*! \brief Current filename substring filter (empty means no filtering). */
    QString name_filter_;
    /*! \brief Current folder path substring filter (empty means no filtering). */
    QString folder_filter_;

    /*! \brief Constructs the proxy model and sets up the parent.
     *
     * \param parent Optional Qt parent object.
     */
    explicit FilterProxyModel(QObject *parent = nullptr);

    /*! \brief Returns a pointer to the set of tags currently used for filtering.
     *
     * \return Pointer to the internal active-filter tag set.
     */
    QSet<Tag*>* getFilterTags();

    /*! \brief Sets the filename substring filter and re-applies the filter.
     *
     * \param filterText The substring to match against file names.
     */
    void setNameFilter(QString filterText);

    /*! \brief Sets the folder path substring filter and re-applies the filter.
     *
     * \param filterText The substring to match against folder paths.
     */
    void setFolderFilter(QString filterText);

    /*! \brief Adds a tag to the active tag filter set and re-applies the filter.
     *
     * \param tag The Tag pointer to add to the filter.
     */
    void addTagFilter(Tag* tag);

    /*! \brief Removes a tag from the active tag filter set and re-applies the filter.
     *
     * \param tag The Tag pointer to remove from the filter.
     */
    void removeTagFilter(Tag* tag);

    /*! \brief Sets the tag-filter mode and re-applies the filter.
     *
     * \param mode AnyTag (OR) or AllTags (AND).
     */
    void setTagFilterMode(TagFilterMode mode);

    /*! \brief Returns the creation-date filter value.
     *
     * \return The currently active creation-date filter.
     */
    QDate getFilterCreationDate() const { return creation_date_; }

    /*! \brief Sets the creation-date filter and re-applies the filter.
     *
     * \param creationDate The date to filter by.
     */
    void setFilterCreationDate(QDate creationDate);

    /*! \brief Sets the rating filter and re-applies the filter.
     *
     * Only files whose rating exactly matches \p rating will pass.
     * Pass std::nullopt to clear the rating filter.
     *
     * \param rating The rating to filter for [1,5], or std::nullopt to disable.
     */
    void setRatingFilter(std::optional<int> rating);

    /*! \brief Sets the comparison mode used when evaluating the rating filter and re-applies the filter.
     *
     * \param mode LessOrEqual, Exactly, or GreaterOrEqual.
     */
    void setRatingFilterMode(RatingFilterMode mode);

    /*! \brief Restricts the visible set to the given files; other filters still apply within the set.
     *
     * \param files The set of TaggedFile pointers to isolate.
     */
    void setIsolationSet(const QSet<TaggedFile*> &files);

    /*! \brief Clears the isolation set so all files are eligible again.
     */
    void clearIsolationSet();

    /*! \brief Returns true when an isolation set is active.
     *
     * \return True if isolation_set_ is non-empty.
     */
    bool isIsolated() const;

    /*! \brief Returns the number of files in the current isolation set.
     *
     * \return Size of isolation_set_, or 0 when not isolated.
     */
    int isolationSetSize() const;

};

#endif // FILTERPROXYMODEL_H
