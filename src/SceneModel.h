#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QAbstractItemModel>
#include <vsg/all.h>
#include <algorithm>

class SceneModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    SceneModel(vsg::ref_ptr<vsg::Group> group, QObject* parent = 0);
    SceneModel(QObject* parent = 0);

    virtual ~SceneModel();

    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex & index ) const;

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

//    void addItem( QObject* propertyObject );

//    void updateItem( QObject* propertyObject, const QModelIndex& parent = QModelIndex() ) ;

    int findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const;

    QModelIndex rootIndexForItem(const vsg::Node *parentNode) { return createIndex(0,0,parentNode); }

    void setRoot(vsg::Group *group);

//    void clear();
    bool hasChildren(const QModelIndex &parent) const;

private:

    enum Columns
        {
            Type,
            Name,
            ColumnCount
        };
    vsg::ref_ptr<vsg::Group> root;
};

#endif // SCENEOBJECT_H
