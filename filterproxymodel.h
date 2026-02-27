#ifndef FILTERPROXYMODEL_H
#define FILTERPROXYMODEL_H

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

private:
    QDate creation_date_;
    QSet<Tag*> tags_ = QSet<Tag*>();
    TagFilterMode tagFilterMode_ = AnyTag;

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

};

#endif // FILTERPROXYMODEL_H
