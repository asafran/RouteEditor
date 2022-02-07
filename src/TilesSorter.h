#ifndef TILESSORTER_H
#define TILESSORTER_H

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

class TilesSorter: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TilesSorter(QObject *parent = nullptr);

public slots:
    void select(const QModelIndex &index);
    void deselect(const QModelIndex &index);
    void expand(const QModelIndex &index);

    void viewSelectSlot(const QItemSelection &selected, const QItemSelection &deselected);
    void viewDoubleClicked(const QModelIndex &index);

signals:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void frontSelectionChanged(const QModelIndex &index);
    void doubleClicked(const QModelIndex &index);

    void viewSelectSignal(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
    void viewExpandSignal(const QModelIndex &index);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
};
#endif // TILESSORTER_H
