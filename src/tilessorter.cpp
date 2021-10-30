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

void TilesSorter::select(const QModelIndex &index)
{
    emit viewSelectSignal(mapFromSource(index), QItemSelectionModel::Select);
}

void TilesSorter::expand(const QModelIndex &index)
{
    emit viewExpandSignal(mapFromSource(index));
}

void TilesSorter::viewSelectSlot(const QItemSelection &selected, const QItemSelection &)
{
    emit selectionChanged(mapToSource(selected.indexes().front()));
}

void TilesSorter::viewDoubleClicked(const QModelIndex &index)
{
    emit doubleClicked(mapToSource(index));
}
