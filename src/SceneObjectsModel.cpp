#include "SceneObjectsModel.h"
#include "sceneobjectvisitor.h"
#include "tile.h"
#include "undo-redo.h"
#include <QMimeData>
#include <sstream>
#include "trajectory.h"
#include <vsg/nodes/LOD.h>
#include <vsg/app/CompileTraversal.h>
#include <vsg/io/VSG.h>

SceneModel::SceneModel(vsg::ref_ptr<route::MVCObject> group, vsg::ref_ptr<vsg::Builder> builder, QObject *parent) :
    QAbstractItemModel(parent)
  , _root(group)
  , _compile(builder->compileTraversal)
  , _options(builder->options)
  , _undoStack(nullptr)
{
}
SceneModel::SceneModel(vsg::ref_ptr<route::MVCObject> group, QObject *parent) :
    QAbstractItemModel(parent)
  , _root(group)
  , _undoStack(nullptr)
{
}

SceneModel::~SceneModel()
{
}

QModelIndex SceneModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!_root || row < 0 || column < 0 || column >= columnCount() ||
            (parent.isValid() && parent.column() != 0) ) {
        return QModelIndex();
    }
    try {
        auto* parentNode = parent.isValid() ? static_cast<route::MVCObject*>(parent.internalPointer()) : _root.get();
        auto node = parentNode->at(row);

        Q_ASSERT(node != nullptr);

        return createIndex(row, column, node);
    }
    catch (std::out_of_range){
        return QModelIndex();
    }
}

QModelIndex SceneModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    auto childNode = static_cast<route::MVCObject*>(child.internalPointer());

    auto parent = childNode->parent();
    if(!parent)
        return QModelIndex();

    auto grandParent = parent->parent();
    if (!grandParent)
        return QModelIndex();

    return createIndex(grandParent->findPos(parent), 0, parent);
}

bool SceneModel::removeRows(int row, int count, const QModelIndex &parent)
{
    auto parentNode = parent.isValid() ? static_cast<route::MVCObject*>(parent.internalPointer()) : _root.get();
    Q_ASSERT(count > 0);

    if (!parentNode->canAdd())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    parentNode->removeChildren(row, count);
    endRemoveRows();

    return true;
}

int SceneModel::addNode(const QModelIndex &parent, vsg::ref_ptr<route::MVCObject> loaded)
{
    int row = rowCount(parent);

    auto parentNode = static_cast<route::MVCObject*>(parent.internalPointer());

    if (!parentNode->canAdd())
        return false;

    beginInsertRows(parent, row, row);
    parentNode->addChild(loaded);
    endInsertRows();
    return row;
}

QModelIndex SceneModel::removeNode(const QModelIndex &index)
{
    auto parent = index.parent();
    removeNode(index, parent);
    return parent;
}
void SceneModel::removeNode(const QModelIndex &index, const QModelIndex &parent)
{
    removeRow(index.row(), parent);
}

QModelIndex SceneModel::index(const route::MVCObject *node) const
{
    auto parent = node->parent();
    if(parent)
        return createIndex(parent->findPos(node), 0, node);
    else
        return QModelIndex();
}

int SceneModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return ColumnCount;
}

Qt::DropActions SceneModel::supportedDropActions() const
{
    return Qt::MoveAction|Qt::CopyAction;
}

QStringList SceneModel::mimeTypes() const
{
    QStringList types;
    types << "text/plain";// << "application/octet-stream";
    return types;
}

QMimeData *SceneModel::mimeData(const QModelIndexList &indexes) const
{
    if(indexes.count() != 1)
        return 0;

    vsg::VSG io;
    _options->extensionHint = "vsgt";

    std::ostringstream oss;

    if (indexes.at(0).isValid()) {
        if(auto node = static_cast<route::MVCObject*>(indexes.at(0).internalPointer()); node)
        {
            io.write(node, oss, _options);
        }
    } else
        return 0;

    QMimeData *mimeData = new QMimeData();
    mimeData->setText(oss.str().c_str());
    return mimeData;
}

bool SceneModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                               int row, int column, const QModelIndex &parent)
{
    if (!data->hasText() || column > 0 || !parent.isValid())
        return false;
    else if (action == Qt::IgnoreAction)
        return true;

    std::istringstream iss(data->text().toStdString());

    vsg::VSG io;

    auto object = io.read(iss, _options);
    auto node = object.cast<route::MVCObject>();
    if(!node)
        return false;

    node->accept(*_compile);

    Q_ASSERT(_undoStack != nullptr);

    _undoStack->push(new AddSceneObject(this, parent, node));

    return true;
}

bool SceneModel::canAdd(const QModelIndex &index) const
{
    if(!index.isValid())
        return false;
    auto node = static_cast<route::MVCObject*>(index.internalPointer());
    return node->canAdd();
}
/*
void SceneModel::fetchMore(const QModelIndex &parent)
{
    beginInsertRows(parent, 0, rowCount(parent));
    endInsertRows();
}
*/

int SceneModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return _root->childrenCount();
    }
    auto parentNode = static_cast<route::MVCObject*>(parent.internalPointer());
    return parentNode->childrenCount();
}
QVariant SceneModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto nodeInfo = static_cast<route::MVCObject*>(index.internalPointer());

    Q_ASSERT(nodeInfo != nullptr);

    switch (index.column()) {
    case Type:
    {
        if (role == Qt::DisplayRole)
            return nodeInfo->className();
        else if(role == Qt::CheckStateRole && index.parent().isValid())
        {
        }
        break;
    }
    case Name:
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            return nodeInfo->getName();
        }
        break;
    }
    case Option:
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
        }
        break;
    }
    default:
        break;
    }
    return QVariant();
}

bool SceneModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid()) {

        auto parentNode = static_cast<route::MVCObject*>(parent.internalPointer());
        return parentNode->childrenCount() > 0;
    }
    return QAbstractItemModel::hasChildren(parent);
}

bool SceneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    auto nodeInfo = static_cast<route::MVCObject*>(index.internalPointer());

    if (role != Qt::EditRole)
        return false;
    if (index.column() == Name)
    {
        QString newName = value.toString();
        QUndoCommand *command = new RenameObject(nodeInfo, newName);

        Q_ASSERT(_undoStack != nullptr);
        _undoStack->push(command);

        emit dataChanged(index, index.sibling(index.row(), ColumnCount));
        return true;
    }
    else if (index.column() == Option)
    {
    }
    return false;
}

QVariant SceneModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const QStringList headers = {tr("Тип"), tr("Имя")};
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < headers.size()) {
        return headers[section];
    }
    return QVariant();
}

Qt::ItemFlags SceneModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (index.isValid())
    {
        switch (index.column()) {
        case Name:
        {
            auto parent = static_cast<route::MVCObject*>(index.parent().internalPointer());
            auto node = static_cast<route::MVCObject*>(index.internalPointer());
            if(parent && !node->is_compatible(typeid(route::Tile)) && !parent->is_compatible(typeid(route::Tile)))
                flags |= Qt::ItemIsEditable ;
            break;
        }
        case Type:
        {
            flags |= Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled|Qt::ItemIsSelectable;
            break;
        }
        case Option:
        {
            break;
        }
        }
    }
    return flags;
}
