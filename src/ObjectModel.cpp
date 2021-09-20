#include "ObjectModel.h"
#include "ParentVisitor.h"


ObjectModel::ObjectModel(QObject *parent) :
    QAbstractItemModel(parent)
{

}



ObjectModel::~ObjectModel()
{}

QModelIndex ObjectModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column > 2) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        auto parentInfo = static_cast<vsg::ref_ptr<vsg::Group>>(parent.internalPointer());
        if (!parentInfo)
            return QModelIndex();
        //auto child = ++std::find(nodePath.begin(), nodePath.end(), parent);
        Q_ASSERT(parentInfo->children.size() < row);
        return createIndex(row, column, &parentInfo->children.at(row));
    }
    switch (row) {
    case NAME :
        return createIndex(row, column, selectedRoot);
    case COORD_ECEF :
        return createIndex(row, column, selectedMatrix);
    case COORD :
        return createIndex(row, column, selectedMatrix);
    case LAYER :
        return createIndex(row, column, tileGroup);
    default:
        return QModelIndex();
    }
}

QModelIndex ObjectModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    auto childInfo = static_cast<vsg::ref_ptr<vsg::Node>>(child.internalPointer());
    Q_ASSERT(childInfo != 0);
    auto parentVisitor = ParentVisitor::create();
    (*nodePath.begin())->accept(*parentVisitor);
    auto parent = parentVisitor->pathToChild.back()->cast<vsg::Group>();
    auto grandParent = (--parentVisitor->pathToChild.back())->cast<vsg::Group>();
    if (parent != 0 ) {
        return createIndex(findRow(grandParent, parent), 0, parent);
    }
    else {
        return QModelIndex();
    }
}

int ObjectModel::findRow(const vsg::Group *parent, const vsg::Node *nodeInfo) const
{
    Q_ASSERT(nodeInfo != 0);
    Q_ASSERT(parent != 0);
    auto position = std::find(parent->children.begin(), parent->children.end(), *nodeInfo);
    Q_ASSERT(position != parent->children.end());
    return std::distance(parent->children.begin(), position);
}

int ObjectModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return 2;
}
