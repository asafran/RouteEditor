#ifndef TILESSORTER_H
#define TILESSORTER_H

#include <QSortFilterProxyModel>

class TilesSorter: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TilesSorter(QObject *parent = nullptr);
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
};
#endif // TILESSORTER_H
