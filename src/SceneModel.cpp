#include "SceneModel.h"
#include "ParentVisitor.h"

SceneModel::SceneModel(vsg::Group* group, QObject *parent) :
    QAbstractItemModel(parent)
  , root(group)
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
            return createIndex(row, column, root->children.at(row));
        }
        auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());

        if(parentNode->is_compatible(typeid (vsg::Group)))
        {
            auto parentGroup = parentNode->cast<vsg::Group>();
            auto child = parentGroup->children.at(row).get();
            return createIndex(row, column, child);
        } else if (parentNode->type_info() == typeid (vsg::PagedLOD))
        {
            auto plod = parentNode->cast<vsg::PagedLOD>();
            return createIndex(row, column, plod->children.at(row).node.get());
        } else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        {
            auto database = parentNode->cast<vsgGIS::TileDatabase>();
            return createIndex(row, column, database->child.get());
        } else {
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
                return createIndex(findRow(grandParent, parent), 0, parent);
        } else if((*parentVisitor->pathToChild.begin())->is_compatible(typeid (vsg::Group))) {

        }
    }
    return QModelIndex();
}

int SceneModel::findRow(const vsg::Node *parentNode, const vsg::Node *childNode) const
{
    Q_ASSERT(childNode != 0);

    if(parentNode->is_compatible(typeid (vsg::Group)))
    {
        auto parent = parentNode->cast<vsg::Group>();
        auto position = std::find(parent->children.cbegin(), parent->children.cend(), vsg::ref_ptr<const vsg::Node>(childNode));
        Q_ASSERT(position != parent->children.end());
        return std::distance(parent->children.begin(), position);

    } else if (parentNode->type_info() == typeid (vsg::PagedLOD)){
        auto plod = parentNode->cast<vsg::PagedLOD>();
        auto it = plod->children.begin();
        for ( ;it != plod->children.end(); ++it)
        {
            if (it->node == childNode)
                break;
        }
        Q_ASSERT(it != plod->children.end());
        return std::distance(plod->children.begin(), it);
    } else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        return 1;
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

int SceneModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return root->children.size();
    }
    if(auto parentNode = static_cast<vsg::Node*>(parent.internalPointer()))
    {
        if(parentNode->is_compatible(typeid (vsg::Group)))
        {
            auto parentGroup = parentNode->cast<vsg::Group>();
            return parentGroup->children.size();
        } else if (parentNode->type_info() == typeid (vsg::PagedLOD))
        {
            auto plod = parentNode->cast<vsg::PagedLOD>();
            return plod->children.size();
        } else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
        {
            return 1;
        }
    }
    return 0;

}
QVariant SceneModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }


    if(auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer()))
    {

        switch (index.column()) {
        case Type: {
            return nodeInfo->className();
        }
        case Name: {
            QString name;
            if(nodeInfo->getValue("Name", name)){
                return name;
            }
            break;
        }
        default:
            break;
        }
    }
    return QVariant();
}

bool SceneModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid()) {

        if (auto parentNode = static_cast<vsg::Node*>(parent.internalPointer()))
        {
            if(parentNode->is_compatible(typeid (vsg::Group)))
            {
                auto parentGroup = parentNode->cast<vsg::Group>();
                return !parentGroup->children.empty();
            } else if (parentNode->type_info() == typeid (vsg::PagedLOD))
            {
                auto plod = parentNode->cast<vsg::PagedLOD>();
                return !plod->children.empty();
            } else if (parentNode->type_info() == typeid (vsgGIS::TileDatabase))
            {
                return true;
            }
            return false;
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
    auto nodeInfo = static_cast<vsg::Node*>(index.internalPointer());
    nodeInfo->setValue("Name", newName.toStdString());

    emit dataChanged(index, index.sibling(index.row(), ColumnCount));
    return true;
}

QVariant SceneModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const QStringList headers = {"Тип", "Имя"};
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
