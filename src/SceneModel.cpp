#include "SceneModel.h"
#include "ParentVisitor.h"

SceneModel::SceneModel(vsg::ref_ptr<vsg::Group> group, QObject *parent) :
    QAbstractItemModel(parent)
  , root(group)
{
}

SceneModel::SceneModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

SceneModel::~SceneModel()
{}

QModelIndex SceneModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!root || row < 0 || column < 0 || column >= columnCount() ||
            (parent.isValid() && parent.column() != 0) ) {
        return QModelIndex();
    }
    try {
        if (!parent.isValid())
        {
            return createIndex(row, column, root.get());
        }
        auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());

        if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
            return createIndex(row, column, parentGroup->children.at(row).get());
        else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
            return createIndex(row, column, plod->children.at(row).node.get());
        /*else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        {
            auto database = parentNode->cast<vsgGIS::TileDatabase>();
            return createIndex(row, column, database->child.get());
        } */else {
            return QModelIndex();
        }
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

    if(auto childInfo = static_cast<vsg::Node*>(child.internalPointer()))
    {

        auto parentVisitor = ParentVisitor::create(childInfo);
        root->accept(*parentVisitor);
        if (parentVisitor->pathToChild.empty())
            return QModelIndex();

        if(parentVisitor->pathToChild.size() > 1)
        {
            auto parent = parentVisitor->pathToChild.back();
            auto grandParent = *(parentVisitor->pathToChild.end() - 2);
            if (parent && grandParent)
                return createIndex(findRow(grandParent, parent), 0, const_cast<vsg::Node*>(parent));
        }
    }
    return QModelIndex();
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
    } /*else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        return 1;*/
    return 0;
}

void SceneModel::setRoot(vsg::Group *group)
{
    root = group;
}

int SceneModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return ColumnCount;
}
/*
Qt::DropActions SceneModel::supportedDropActions() const
{
    return Qt::MoveAction|Qt::CopyAction;
}
*/
int SceneModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return root->children.size();
    }
    if(auto parentNode = static_cast<vsg::Node*>(parent.internalPointer()))
    {
        if(auto parentGroup = parentNode->cast<vsg::Group>(); parentGroup)
            return parentGroup->children.size();
        else if (auto plod = parentNode->cast<vsg::PagedLOD>(); plod)
            return plod->children.size();
        /*else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        {
            return 1;
        }*/
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
            /*else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
            {
                return true;
            }*/
        }
    }
    return QAbstractItemModel::hasChildren(parent);
}

bool SceneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (role != Qt::EditRole) {
        return false;
    }
    if (index.column() != Name) {
        return false;
    }

    QString newName = value.toString();
    auto object = static_cast<vsg::Object*>(index.internalPointer());
    QUndoCommand *command = new RenameObject(object, newName);
    emit renameObject(command);

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
    if (index.isValid() && index.column() == Name) {
        //auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer());
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}
