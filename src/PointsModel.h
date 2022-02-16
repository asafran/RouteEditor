#ifndef POINTSMODEL_H
#define POINTSMODEL_H

#include <QUndoStack>
#include <QAbstractTableModel>
#include <vsg/core/Object.h>

namespace route {
    class SplineTrajectory;
}

class PointsModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    explicit PointsModel(route::SplineTrajectory *trj, QUndoStack *stack, QObject* parent = 0);

    ~PointsModel();

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    //bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    //void clear();

    //void setTraj(route::SplineTrajectory *trj);

private:

    enum Properties
    {
        Incliantion,
        Tangent,
        Tilt,
        CatenaryHeight,
        ColumnCount
    };
    vsg::ref_ptr<route::SplineTrajectory> _trajectory;

    QUndoStack *_undoStack;
};

#endif // POINTSMODEL_H
