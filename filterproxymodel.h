#ifndef FILTERPROXYMODEL_H
#define FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>
#include <QSet>
#include "taggedfile.h"

class FilterProxyModel : public QSortFilterProxyModel
{
private:
    QDate creation_date_;
    QSet<Tag*> tags_ = QSet<Tag*>();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

public:
    QString name_filter_;
    explicit FilterProxyModel(QObject *parent = nullptr);

    QSet<Tag*>* getFilterTags();

    void setNameFilter(QString filterText);
    void addTagFilter(Tag* tag);
    void removeTagFilter(Tag* tag);

    QDate getFilterCreationDate() const { return creation_date_; }
    void setFilterCreationDate(QDate creationDate);

};

#endif // FILTERPROXYMODEL_H
