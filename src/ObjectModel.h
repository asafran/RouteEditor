#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <algorithm>
#include "sceneobjects.h"
#include <QUndoStack>


class ObjectModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    ObjectModel(vsg::ref_ptr<vsg::EllipsoidModel> ellipsoid, QUndoStack *stack, QObject* parent = 0);

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    void clear();

public slots:
    void selectObject(const QModelIndex &modelindex);

private:
/*
    typedef QVector<NodeInfo> NodeInfoList;
    NodeInfoList _nodes;
*/
    //vsg::dmat4 rotate(double yaw, double pitch, double roll);
    enum Properties
    {
        NAME = 0,
        COORD_ECEFX = 1,
        COORD_ECEFY = 2,
        COORD_ECEFZ = 3,
        COORDX = 4,
        COORDY = 5,
        COORDZ = 6,
        ROTATEX = 7,
        ROTATEY = 8,
        ROTATEZ = 9,
        //SHADER = 11,
        //ANIMATION = 12,
        ROW_COUNT = 10
    };
    vsg::ref_ptr<SceneObject> selectedObject;
    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel;
    QUndoStack *undoStack;
};

#endif // SCENEOBJECT_H
