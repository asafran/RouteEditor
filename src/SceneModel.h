#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QAbstractItemModel>
#include <QUndoStack>
#include <vsg/all.h>
#include <algorithm>
#include <QFileInfo>

class SceneObject : public vsg::Inherit<vsg::MatrixTransform, SceneObject>
{
public:
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file, const vsg::dmat4& in_matrix = {}) : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix)
    {
        addChild(loaded);
        setValue(META_NAME, file.baseName().toStdString());
        path = file.absoluteFilePath();
    }

    SceneObject(vsg::Allocator* allocator = nullptr) : vsg::Inherit<vsg::MatrixTransform, SceneObject>(allocator) {}

    ~SceneObject() {}

    QString path;
    QModelIndex index;
};

class SceneGroup : public vsg::Inherit<vsg::MatrixTransform, SceneGroup>
{
public:
    SceneGroup(const std::string& name, const vsg::dmat4& in_matrix = {}) : vsg::Inherit<vsg::MatrixTransform, SceneGroup>(in_matrix)
    {
        setValue(META_NAME, name);
    }

    SceneGroup(vsg::Allocator* allocator = nullptr) : vsg::Inherit<vsg::MatrixTransform, SceneGroup>(allocator) {}

    ~SceneGroup() {}

//    QModelIndex index;
};

class SceneModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    SceneModel(vsg::ref_ptr<vsg::Group> group, QObject* parent = 0);
    SceneModel(vsg::ref_ptr<vsg::Group> group, QUndoStack *stack, QObject* parent = 0);

    virtual ~SceneModel();

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

    void addNode(const QModelIndex &parent, QUndoCommand *command);

    int findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const;

    QModelIndex index(vsg::ref_ptr<vsg::Node> node, int row);

//    void clear();
    bool hasChildren(const QModelIndex &parent) const;

signals:
    void sendCommand(QUndoCommand *command);
    void sendMap(QMap<vsg::Group *, QModelIndex> groupMap);

private:

    enum Columns
        {
            Type,
            Name,
            ColumnCount
        };
    vsg::ref_ptr<vsg::Group> root;
    QUndoStack *undoStack;
};

#endif // SCENEOBJECT_H
