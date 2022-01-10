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

    explicit SceneModel(vsg::ref_ptr<vsg::Group> group, QObject* parent = 0);
    SceneModel(vsg::ref_ptr<vsg::Group> group, vsg::ref_ptr<vsg::Builder> compile, QUndoStack *stack, QObject* parent = 0);

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

//    void fetchMore(const QModelIndex &parent);

    /*
    template<typename F1, typename... Args>
    void fetchMore(const QModelIndex &parent, F1 function1, Args&... args)
    {
        int rows = rowCount(parent);
        function1(root, args...);
        beginInsertRows(parent, rows, rowCount(parent));
        endInsertRows();
    }*/

    int addNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> loaded, uint32_t mask = route::SceneObjects);
    /*
    uint32_t setMask(uint32_t mask, int row, const QModelIndex &parent);
    uint32_t setMask(uint32_t mask, const QModelIndex &index);
    */
    QModelIndex removeNode(const QModelIndex &index);
    void removeNode(const QModelIndex &parent, const QModelIndex &index);
    //void removeNode(vsg::ref_ptr<vsg::Node> node);
    //void removeNode(const QModelIndex &parent, int row, vsg::ref_ptr<vsg::Node> node);

    //int findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const;

    QModelIndex index(const vsg::Node *node) const;
    QModelIndex index(const vsg::Node *node, const vsg::Node *parent) const;

//    void clear();
    bool hasChildren(const QModelIndex &parent) const;

    vsg::ref_ptr<vsg::Group> getRoot() { return _root; }

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
            Option,
            ColumnCount
        };

    vsg::ref_ptr<vsg::Group> _root;
    vsg::ref_ptr<vsg::Builder> _compile;
    QUndoStack *_undoStack;
};

#endif // SCENEMODEL_H
