#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QAbstractItemModel>
#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/Group.h>
#include <vsg/traversals/Intersector.h>
#include <algorithm>
#include <vsg/maths/transform.h>

class ObjectModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    ObjectModel(QObject* parent = 0);

    virtual ~ObjectModel();


    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex & index ) const;

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QModelIndex buddy ( const QModelIndex & index ) const;

    void addItem( QObject* propertyObject );

    void updateItem( QObject* propertyObject, const QModelIndex& parent = QModelIndex() ) ;

    int findRow(const vsg::Group *parent, const vsg::Node *nodeInfo) const;

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
        LAYER = 4,
        ANIMATION = 5
    };
    vsg::ref_ptr<vsg::MatrixTransform> selectedMatrix;

    vsg::Intersector::NodePath nodePath;
    vsg::ref_ptr<vsg::Group> selectedRoot;
    vsg::ref_ptr<vsg::Group> tileGroup;
};

#endif // SCENEOBJECT_H
