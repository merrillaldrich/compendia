#ifndef FILTERPROXYMODEL_H
#define FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>
#include "taggedfile.h"

class FilterProxyModel : public QSortFilterProxyModel
{
private:
    QDate creation_date_;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

public:
    QString name_filter_;
    explicit FilterProxyModel(QObject *parent = nullptr);

    void setNameFilter(QString filterText);

    QDate getFilterCreationDate() const { return creation_date_; }
    void setFilterCreationDate(QDate creationDate);

};

#endif // FILTERPROXYMODEL_H
