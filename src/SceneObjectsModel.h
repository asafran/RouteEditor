#ifndef SCENEMODEL_H
#define SCENEMODEL_H

#include <QUndoStack>
#include <QAbstractItemModel>
#include "sceneobjects.h"
#include <vsg/utils/Builder.h>

class SceneModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    explicit SceneModel(vsg::ref_ptr<route::MVCObject> group, QObject* parent = 0);
    SceneModel(vsg::ref_ptr<route::MVCObject> group, vsg::ref_ptr<vsg::Builder> builder, QObject* parent = 0);

    ~SceneModel();

    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex & index ) const;

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    bool removeRows(int row, int count, const QModelIndex &parent);

    Qt::DropActions supportedDropActions() const;

    QStringList mimeTypes() const;

    QMimeData *mimeData(const QModelIndexList &indexes) const;

    //bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    bool canAdd( const QModelIndex & index ) const;

    int addNode(const QModelIndex &parent, vsg::ref_ptr<route::MVCObject> loaded);

    QModelIndex removeNode(const QModelIndex &index);
    void removeNode(const QModelIndex &index, const QModelIndex &parent);

    QModelIndex index(const route::MVCObject *node) const;

    bool hasChildren(const QModelIndex &parent) const;

    vsg::ref_ptr<route::MVCObject> getRoot() { return _root; }

    void setUndoStack(QUndoStack *stack) { _undoStack = stack; }

private:

    enum Columns
        {
            Type,
            Name,
            Option,
            ColumnCount
        };

    vsg::ref_ptr<route::MVCObject> _root;
    vsg::ref_ptr<vsg::CompileTraversal> _compile;
    vsg::ref_ptr<vsg::Options> _options;

    QUndoStack *_undoStack;
};

#endif // SCENEMODEL_H
