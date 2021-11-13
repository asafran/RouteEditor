#ifndef SCENEMODEL_H
#define SCENEMODEL_H

#include "ObjectModel.h"
#include <QUndoStack>
#include "sceneobjects.h"

class SceneModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    explicit SceneModel(vsg::ref_ptr<vsg::Group> group, QObject* parent = 0);
    SceneModel(vsg::ref_ptr<vsg::Group> group, vsg::ref_ptr<vsg::Builder> builder, QUndoStack *stack, QObject* parent = 0);

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

//    bool canFetchMore(const QModelIndex &parent) const;

    void fetchMore(const QModelIndex &parent);

    template<typename F1, typename... Args>
    void fetchMore(const QModelIndex &parent, F1 function1, Args&... args)
    {
        int rows = rowCount(parent);
        function1(root, args...);
        beginInsertRows(parent, rows, rowCount(parent));
        endInsertRows();
    }

    void addNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> node);
    void removeNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> node);

    int findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const;

    QModelIndex index(const vsg::Node *node) const;

//    void clear();
    bool hasChildren(const QModelIndex &parent) const;

    vsg::ref_ptr<vsg::Group> getRoot() { return root; }

/*
signals:
    void sendCommand(QUndoCommand *command);
    void sendMap(QMap<vsg::Group *, QModelIndex> groupMap);
*/

private:

    enum Columns
        {
            Type,
            Name,
            ColumnCount
        };
    vsg::ref_ptr<vsg::Group> root;
     vsg::ref_ptr<vsg::Builder> compiler;
    QUndoStack *undoStack;
};

#endif // SCENEMODEL_H
