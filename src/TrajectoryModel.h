#ifndef TRAJMODEL_H
#define TRAJMODEL_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <algorithm>
#include "sceneobjects.h"
#include <QUndoStack>


class TrajectoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TrajectoryModel(QUndoStack *stack, QObject* parent = 0);

    ~TrajectoryModel();

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    void clear();

public slots:
    void selectObject(const QModelIndex &modelindex);
    //void rotateVertical();

private:
/*
    typedef QVector<NodeInfo> NodeInfoList;
    NodeInfoList _nodes;
*/
    //vsg::dmat4 rotate(double yaw, double pitch, double roll);
    enum Properties
    {
        INCLINATION,
        SUPERELEVATION,
        VIBRATION,
        COLUMN_COUNT
    };
    vsg::ref_ptr<Trajectory> selectedObject;
    QUndoStack *undoStack;
};

#endif // TRAJMODEL_H
