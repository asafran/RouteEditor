#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QAbstractItemModel>
#include <algorithm>
#include "sceneobjects.h"

class ObjectModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    ObjectModel(QObject* parent = 0);

    virtual ~ObjectModel();

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    void clear();

private:
/*
    typedef QVector<NodeInfo> NodeInfoList;
    NodeInfoList _nodes;
*/
    enum Properties
    {
        NAME = 0,
        COORD_ECEF = 2,
        COORD = 3,
        PARENT = 4,
        SHADER = 5,
        ANIMATION = 6,
        ROW_COUNT = 7
    };
    vsg::ref_ptr<SceneObject> selectedObject;
};

#endif // SCENEOBJECT_H
