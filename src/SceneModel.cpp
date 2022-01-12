#include "SceneModel.h"
#include "ParentVisitor.h"
#include "LambdaVisitor.h"
#include "SceneObjectVisitor.h"
#include "undo-redo.h"
#include <QMimeData>
#include <sstream>
#include "trajectory.h"
#include <vsg/nodes/LOD.h>
#include <vsg/traversals/CompileTraversal.h>
#include <vsg/io/VSG.h>

SceneModel::SceneModel(vsg::ref_ptr<vsg::Group> group, vsg::ref_ptr<vsg::Builder> compile, QUndoStack *stack, QObject *parent) :
    QAbstractItemModel(parent)
  , _root(group)
  , _compile(compile)
  , _undoStack(stack)
{
}
SceneModel::SceneModel(vsg::ref_ptr<vsg::Group> group, QObject *parent) :
    QAbstractItemModel(parent)
  , _root(group)
  , _undoStack(Q_NULLPTR)
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
        vsg::Node* parentNode = parent.isValid() ? static_cast<vsg::Node*>(parent.internalPointer()) : _root.get();

        vsg::Node* child = 0;

        auto autoF = [&child, row](const auto& node) { child = node.children.at(row).node; };
        auto groupF = [&child, row](const vsg::Group& node) { child = node.children.at(row); };
        auto swF = [&child, row](const vsg::Switch& node)
        {
            auto object = std::find_if(node.children.begin(), node.children.end(), [](const vsg::Switch::Child &ch) { return (ch.mask & route::SceneObjects) != 0;} );
            object += row;
            Q_ASSERT(object < node.children.cend());
            child = object->node;
        };

        CFunctionVisitor<decltype (autoF)> fv(autoF);
        fv.groupFunction = groupF;
        fv.swFunction = swF;

        parentNode->accept(fv);

        return createIndex(row, column, child);
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

    auto childInfo = static_cast<vsg::Node*>(child.internalPointer());

    Q_ASSERT(childInfo != nullptr);

    ParentVisitor parentVisitor(childInfo);
    parentVisitor.traversalMask = route::SceneObjects;
    _root->accept(parentVisitor);
    if (parentVisitor.pathToChild.size() < 3)
        return QModelIndex();
    auto parent = *(parentVisitor.pathToChild.end() - 2);
    auto grandParent = *(parentVisitor.pathToChild.end() - 3);
    if (parent && grandParent)
    {
        FindPositionVisitor fpv(parent);
        fpv.traversalMask = route::SceneObjects;
        return createIndex(fpv(grandParent), 0, const_cast<vsg::Node*>(parent));
    }
    return QModelIndex();
}

bool SceneModel::removeRows(int row, int count, const QModelIndex &parent)
{
    vsg::Node* parentNode = parent.isValid() ? static_cast<vsg::Node*>(parent.internalPointer()) : _root.get();
    Q_ASSERT(count > 0);

    if (parentNode->is_compatible(typeid (vsg::PagedLOD)))
        return false;

    auto autoF = [row, count](auto& node)
    {
        auto begin = node.children.cbegin() + row;
        auto end = begin + count - 1;
        Q_ASSERT(end < node.children.end());
        if(begin == end)
            node.children.erase(begin);
        else
            node.children.erase(begin, end);
    };
    auto groupF = [autoF](vsg::Group& node) { autoF(node); };
    auto lodF = [autoF](vsg::LOD& node) { autoF(node); };

    auto swF = [row, count](vsg::Switch& node)
    {
        auto begin = std::find_if(node.children.begin(), node.children.end(), [](const vsg::Switch::Child &ch)
        {
            return (ch.mask & route::SceneObjects) != 0;}
        );
        begin += row;
        auto end = begin + count - 1;
        Q_ASSERT(end < node.children.end());
        if(begin == end)
            node.children.erase(begin);
        else
            node.children.erase(begin, end);
    };

    FunctionVisitor fv(groupF, swF, lodF);
    beginRemoveRows(parent, row, row + count - 1);
    parentNode->accept(fv);
    endRemoveRows();

    return true;

}

int SceneModel::addNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> loaded, uint32_t mask)
{
    int row = rowCount(parent);

    vsg::Node* parentNode = static_cast<vsg::Node*>(parent.internalPointer());

    if (parentNode->is_compatible(typeid (vsg::PagedLOD)))
        return false;

    auto groupF = [loaded](vsg::Group& group) { group.addChild(loaded); };
    auto swF = [loaded, mask](vsg::Switch& sw) { sw.addChild(mask, loaded); };
    auto lodF = [loaded](vsg::LOD& node) { node.addChild(vsg::LOD::Child{0.0, loaded}); };

    beginInsertRows(parent, row, row);
    FunctionVisitor fv(groupF, swF);
    parentNode->accept(fv);
    endInsertRows();
    return row;
}
/*
uint32_t SceneModel::setMask(uint32_t mask, int row, const QModelIndex &parent)
{
    auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());
    auto swF = [mask](vsg::Switch& sw) { sw.addChild(true, loaded); };
}

uint32_t SceneModel::setMask(uint32_t mask, const QModelIndex &index)
{
    auto parentNode = static_cast<vsg::Node*>(index.parent().internalPointer());
    auto node = static_cast<vsg::Node*>(index.internalPointer());
    FindPositionVisitor fpv(node);
    auto row = fpv(parentNode);
    auto swF = [mask, row](vsg::Switch& sw) { sw.children.at(row).mask = mask; };

    FunctionVisitor fv;
    fv._swFunction = swF;
    parentNode->accept(fv);
}*/

QModelIndex SceneModel::removeNode(const QModelIndex &index)
{
    //FindPositionVisitor fpv(static_cast<vsg::Node*>(index.internalPointer()));
    //fpv.traversalMask = route::SceneObjects;
    auto parent = index.parent();
    //removeRows(fpv(static_cast<vsg::Node*>(index.parent().internalPointer())), 1, parent);
    removeRow(index.row(), parent);
    return parent;
}
void SceneModel::removeNode(const QModelIndex &parent, const QModelIndex &index)
{
    //FindPositionVisitor fpv(static_cast<vsg::Node*>(index.internalPointer()));
    //fpv.traversalMask = route::SceneObjects;
    //removeRows(fpv(static_cast<vsg::Node*>(index.parent().internalPointer())), 1, parent);
    removeRow(index.row(), parent);
}

QModelIndex SceneModel::index(const vsg::Node *node) const
{
    ParentVisitor parentVisitor(node);
    parentVisitor.traversalMask = route::SceneObjects;
    _root->accept(parentVisitor);
    if (parentVisitor.pathToChild.empty() || parentVisitor.pathToChild.size() < 2)
        return QModelIndex();
    auto parent = *(parentVisitor.pathToChild.end() - 2);

    return SceneModel::index(node, parent);
}

QModelIndex SceneModel::index(const vsg::Node *node, const vsg::Node *parent) const
{
    FindPositionVisitor fpv(node);
    fpv.traversalMask = route::SceneObjects;
    return createIndex(fpv(parent), 0, node);
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
    _compile->options->extensionHint = "vsgt";

    std::ostringstream oss;

    if (indexes.at(0).isValid()) {
        if(auto node = static_cast<vsg::Node*>(indexes.at(0).internalPointer()); node)
        {
            io.write(node, oss, _compile->options);
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

    auto object = io.read(iss, _compile->options);
    auto node = object.cast<vsg::Node>();
    if(!node)
        return false;

    _compile->compile(node);

    CalculateTransform ct;
    node->accept(ct);

    _undoStack->push(new AddSceneObject(this, parent, node));

    return true;
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
        return _root->children.size();
    }
    auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());

    int rows = 0;

    auto autoF = [&rows](const auto& node) { rows = node.children.size(); };
    auto swF = [&rows](const vsg::Switch& node)
    {
        rows = std::count_if(node.children.begin(), node.children.end(), [](const vsg::Switch::Child &child)
        {
            return ((child.mask & route::SceneObjects) != 0);
        });
    };

    CFunctionVisitor<decltype (autoF)> fv(autoF);
    fv.groupFunction = autoF;
    fv.swFunction = swF;

    parentNode->accept(fv);

    return rows;
}
QVariant SceneModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer());

    //Q_ASSERT(nodeInfo != nullptr);

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
            std::string name;
            if(nodeInfo->getValue(META_NAME, name))
                return name.c_str();
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

        auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());

        Q_ASSERT(parentNode != nullptr);

        bool has = false;
        auto autoF = [&has](const auto &node) { has = !node.children.empty(); };
        //auto trajF = [&has](const SceneTrajectory& node) { has = node.traj->size() != 0; };

        CFunctionVisitor<decltype (autoF)> fv(autoF);
        fv.groupFunction = autoF;
        parentNode->accept(fv);
        return has;
    }
    return QAbstractItemModel::hasChildren(parent);
}

bool SceneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer());

    if(role == Qt::CheckStateRole && index.parent().isValid())
    {
        /*
        auto visit = [index, value](vsg::Switch& node)
        {
            node.children.at(index.row()).enabled = value.toBool();
        };
        Lambda1Visitor<decltype (visit), vsg::Switch> lv(visit);
        static_cast<vsg::Node*>(index.parent().internalPointer())->accept(lv);
        */
        return false;
    }
    if (role != Qt::EditRole)
        return false;
    if (index.column() == Name)
    {
        QString newName = value.toString();
        QUndoCommand *command = new RenameObject(nodeInfo, newName);

        Q_ASSERT(_undoStack != Q_NULLPTR);
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
            if(parent(index).isValid())
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
