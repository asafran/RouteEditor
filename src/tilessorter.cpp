#include "tilessorter.h"

TilesSorter::TilesSorter(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool TilesSorter::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    bool result = QSortFilterProxyModel::filterAcceptsRow(source_row,source_parent);
    QModelIndex currentIndex = sourceModel()->index(source_row, 0, source_parent);
    if (sourceModel()->hasChildren(currentIndex))
        for (int i = 0; i < sourceModel()->rowCount(currentIndex) && !result; ++i)
            result = result || filterAcceptsRow(i, currentIndex);
    return result;
}
