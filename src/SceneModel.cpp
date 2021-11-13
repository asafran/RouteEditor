#include "SceneModel.h"
#include "ParentVisitor.h"
#include "undo-redo.h"
#include <QMimeData>
#include <sstream>

SceneModel::SceneModel(vsg::ref_ptr<vsg::Group> group, vsg::ref_ptr<vsg::Builder> builder, QUndoStack *stack, QObject *parent) :
    QAbstractItemModel(parent)
  , root(group)
  , compiler(builder)
  , undoStack(stack)
{
}
SceneModel::SceneModel(vsg::ref_ptr<vsg::Group> group, QObject *parent) :
    QAbstractItemModel(parent)
  , root(group)
  , undoStack(Q_NULLPTR)
{
}

SceneModel::~SceneModel()
{
}

QModelIndex SceneModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!root || row < 0 || column < 0 || column >= columnCount() ||
            (parent.isValid() && parent.column() != 0) ) {
        return QModelIndex();
    }
    try {
        vsg::Node* parentNode = parent.isValid() ? static_cast<vsg::Node*>(parent.internalPointer()) : root.get();

        if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
            return createIndex(row, column, parentGroup->children.at(row).get());
        else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
            return createIndex(row, column, plod->children.at(row).node.get());
        else if (auto sw = parentNode->cast<vsg::Switch>(); sw)
            return createIndex(row, column, sw->children.at(row).node.get());
        else
            return QModelIndex();
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

    if(auto childInfo = static_cast<vsg::Node*>(child.internalPointer()); childInfo)
    {
        auto parentVisitor = ParentVisitor::create(childInfo);
        root->accept(*parentVisitor);
        if (parentVisitor->pathToChild.size() < 2)
            return QModelIndex();
        auto parent = parentVisitor->pathToChild.back();
        auto grandParent = *(parentVisitor->pathToChild.end() - 2);
        if (parent && grandParent)
            return createIndex(findRow(grandParent, parent), 0, const_cast<vsg::Node*>(parent));
    }
    return QModelIndex();
}

bool SceneModel::removeRows(int row, int count, const QModelIndex &parent)
{
    vsg::Node* parentNode = parent.isValid() ? static_cast<vsg::Node*>(parent.internalPointer()) : root.get();
    Q_ASSERT(count > 0);
    if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
    {
        Q_ASSERT(parentGroup->children.size() >= row + count - 1);
        beginRemoveRows(parent, row, row + count - 1);

        auto it = parentGroup->children.cbegin() + row;
        auto count_iterator = it + count;

        for( ; it != count_iterator; ++it)
            parentGroup->children.erase(it);
        endRemoveRows();
    }
    else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
    {
        return false;
    }
    else if(auto parentSwitch = parentNode->cast<vsg::Switch>(); parentSwitch)
    {
        Q_ASSERT(parentSwitch->children.size() >= row + count - 1);
        beginRemoveRows(parent, row, row + count - 1);

        auto it = parentSwitch->children.cbegin() + row;
        auto count_iterator = it + count;

        for( ; it != count_iterator; ++it)
            parentSwitch->children.erase(it);
        endRemoveRows();
    }
    return true;
}

int SceneModel::findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const
{
    Q_ASSERT(childNode != 0);

    if(auto parent = parentNode->cast<vsg::Group>(); parent)
    {
        auto position = std::find(parent->children.cbegin(), parent->children.cend(), vsg::ref_ptr<const vsg::Node>(childNode));
        Q_ASSERT(position != parent->children.end());
        return std::distance(parent->children.begin(), position);
    } else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
    {
        auto it = plod->children.begin();
        for ( ;it != plod->children.end(); ++it)
        {
            if (it->node == childNode)
                break;
        }
        Q_ASSERT(it != plod->children.end());
        return std::distance(plod->children.begin(), it);
    } else if (auto sw = parentNode->cast<vsg::Switch>(); sw)
    {
        auto it = sw->children.begin();
        for ( ;it != sw->children.end(); ++it)
        {
            if (it->node == childNode)
                break;
        }
        Q_ASSERT(it != sw->children.end());
        return std::distance(sw->children.begin(), it);
    }
    return 0;
}
void SceneModel::addNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> node)
{
    int row = rowCount(parent);
    if(auto group = static_cast<vsg::Node*>(parent.internalPointer())->cast<vsg::Group>(); group)
    {
        beginInsertRows(parent, row, row);
        group->addChild(node);
        endInsertRows();
    } else if(auto sw = static_cast<vsg::Node*>(parent.internalPointer())->cast<vsg::Switch>(); sw)
    {
        beginInsertRows(parent, row, row);
        sw->addChild(true, node);
        endInsertRows();
    }
}

void SceneModel::removeNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> node)
{
    int row = findRow(static_cast<vsg::Node*>(parent.internalPointer()), node);
    removeRows(row, 1, parent);
}
QModelIndex SceneModel::index(const vsg::Node *node) const
{
    auto parentVisitor = ParentVisitor::create(node);
    root->accept(*parentVisitor);
    if (parentVisitor->pathToChild.empty())
        return QModelIndex();
    auto parent = parentVisitor->pathToChild.back();
    return createIndex(findRow(parent, node), 0, node);
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
    types << "text/plain" << "application/octet-stream";
    return types;
}

QMimeData *SceneModel::mimeData(const QModelIndexList &indexes) const
{
    Q_ASSERT(indexes.count());
    if(indexes.count() != 1)
        return 0;

    vsg::VSG io;
    auto options = vsg::Options::create();
    options->extensionHint = "vsgt";

    std::ostringstream oss;

    if (indexes.at(0).isValid()) {
        if(auto node = static_cast<vsg::Node*>(indexes.at(0).internalPointer()); node)
        {
            io.write(node, oss, options);
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
    if (!data->hasText())
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    if (!parent.isValid())
        return false;

    auto options = vsg::Options::create();
    options->extensionHint = "vsgt";

    std::istringstream iss(data->text().toStdString());

    vsg::VSG io;

    auto object = io.read(iss, options);
    auto node = object.cast<vsg::Node>();
    Q_ASSERT(compiler);
    compiler->compile(node);
    if(!node)
        return false;
    undoStack->push(new AddNode(this, parent, node));

    return true;
}

void SceneModel::fetchMore(const QModelIndex &parent)
{
    beginInsertRows(parent, 0, rowCount(parent));
    endInsertRows();
}


int SceneModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return root->children.size();
    }
    if(auto parentNode = static_cast<vsg::Node*>(parent.internalPointer()); parentNode)
    {
        if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
            return parentGroup->children.size();
        else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
            return plod->children.size();
        else if (auto sw = parentNode->cast<vsg::Switch>(); sw)
            return sw->children.size();
    }
    return 0;

}
QVariant SceneModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if(auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer()); nodeInfo)
    {
        switch (index.column()) {
            case Type:
                if (role == Qt::DisplayRole) {
                    return nodeInfo->className();
                }
                else if(role == Qt::CheckStateRole && index.parent().isValid())
                {
                    if(auto sw = static_cast<vsg::Node*>(index.parent().internalPointer())->cast<vsg::Switch>(); sw)
                    {
                        return sw->children.at(findRow(sw, nodeInfo)).enabled ? Qt::Checked : Qt::Unchecked;
                    }
                }
            case Name:
                if (role == Qt::DisplayRole) {
                    std::string name;
                    if(nodeInfo->getValue(META_NAME, name))
                        return name.c_str();
                }
                break;
            default:
                break;
            }
            return QVariant();
    }
    return QVariant();
}

bool SceneModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid()) {

        if (auto parentNode = static_cast<vsg::Node*>(parent.internalPointer()); parentNode)
        {
            if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
                return !parentGroup->children.empty();
            else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
                return !plod->children.empty();
            else if (auto sw = parentNode->cast<vsg::Switch>(); sw)
                return !sw->children.empty();
        }
    }
    return QAbstractItemModel::hasChildren(parent);
}

bool SceneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if(role == Qt::CheckStateRole && index.parent().isValid())
    {
        if(auto sw = static_cast<vsg::Node*>(index.parent().internalPointer())->cast<vsg::Switch>(); sw)
        {
            auto row = findRow(sw, static_cast<vsg::Node*>(index.internalPointer()));
            sw->children.at(row).enabled = value.toBool();
            return true;
        }
    }
    if (index.column() != Name || role != Qt::EditRole)
        return false;

    QString newName = value.toString();
    auto object = static_cast<vsg::Object*>(index.internalPointer());
    QUndoCommand *command = new RenameObject(object, newName);

    Q_ASSERT(undoStack != Q_NULLPTR);
    undoStack->push(command);

    emit dataChanged(index, index.sibling(index.row(), ColumnCount));
    return true;
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
        flags |= Qt::ItemIsSelectable;

        switch (index.column()) {
        case Name:
        {
            if(parent(index).isValid())
                flags |= Qt::ItemIsEditable;
            break;
        }
        case Type:
        {
            if(index.parent().isValid() && static_cast<vsg::Node*>(index.parent().internalPointer())->is_compatible(typeid (vsg::Switch)))
                flags |= Qt::ItemIsUserCheckable;
            flags |= Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled;
            break;
        }
        }
    }
    return flags;
}
