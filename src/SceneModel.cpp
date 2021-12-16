#include "SceneModel.h"
#include "ParentVisitor.h"
#include "LambdaVisitor.h"
#include "undo-redo.h"
#include <QMimeData>
#include <sstream>
#include "trajectory.h"

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

        vsg::Node* child = 0;

        auto autoF = [&child, row](const auto& node) { child = node.children.at(row).node; };
        auto groupF = [&child, row](const vsg::Group& node) { child = node.children.at(row); };
        //auto trajF = [&child, row, this](const SceneTrajectory& node) { child = *(node.traj->getBegin()+row); };

        CFunctionVisitor<decltype (autoF)> fv(autoF, groupF);
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
    root->accept(parentVisitor);
    if (parentVisitor.pathToChild.size() < 3)
        return QModelIndex();
    auto parent = *(parentVisitor.pathToChild.end() - 2);
    auto grandParent = *(parentVisitor.pathToChild.end() - 3);
    if (parent && grandParent)
    {
        FindPositionVisitor fpv(parent);
        return createIndex(fpv(grandParent), 0, const_cast<vsg::Node*>(parent));
    }
    return QModelIndex();
}

bool SceneModel::removeRows(int row, int count, const QModelIndex &parent)
{
    vsg::Node* parentNode = parent.isValid() ? static_cast<vsg::Node*>(parent.internalPointer()) : root.get();
    Q_ASSERT(count > 0);

    if (parentNode->is_compatible(typeid (vsg::PagedLOD)))
        return false;

    auto autoF = [row, count](auto& node)
    {
        auto it = node.children.cbegin() + row;
        auto count_iterator = it + count;

        for( ; it != count_iterator; ++it)
            node.children.erase(it);
    };
    auto groupF = [autoF](vsg::Group& node) { autoF(node); };
    auto swF = [autoF](vsg::Switch& node) { autoF(node); };
    auto lodF = [autoF](vsg::LOD& node) { autoF(node); };
    /*
    auto trajF = [row, count](SceneTrajectory& node)
    {
        count = 1;
        row = node.traj->size() - 1;
        node.traj->removeTrack();
    };*/

    FunctionVisitor fv(groupF, swF, lodF);
    beginRemoveRows(parent, row, row + count - 1);
    parentNode->accept(fv);
    endRemoveRows();

    return true;

}

int SceneModel::addNode(const QModelIndex &parent, vsg::ref_ptr<vsg::Node> loaded)
{
    int row = rowCount(parent);
    beginInsertRows(parent, row, row);
    if(auto group = static_cast<vsg::Node*>(parent.internalPointer())->cast<vsg::Group>(); group)
        group->addChild(loaded);

    else if(auto sw = static_cast<vsg::Node*>(parent.internalPointer())->cast<vsg::Switch>(); sw)
        sw->addChild(true, loaded);

    vsg::Node* parentNode = static_cast<vsg::Node*>(parent.internalPointer());

    if (parentNode->is_compatible(typeid (vsg::PagedLOD)))
        return false;

    auto groupF = [loaded](vsg::Group& group) { group.addChild(loaded); };
    auto swF = [loaded](vsg::Switch& sw) { sw.addChild(true, loaded); };
    //auto lodF = [loaded](vsg::LOD& node) { autoF(node); };

    /*auto trajF = [loaded](SceneTrajectory& node)
    {
        node.traj->removeTrack();
    };*/

    FunctionVisitor fv(groupF, swF);
    endInsertRows();
    return row;
}

QModelIndex SceneModel::removeNode(const QModelIndex &index)
{
    FindPositionVisitor fpv(static_cast<vsg::Node*>(index.internalPointer()));
    auto parent = index.parent();
    removeRows(fpv(static_cast<vsg::Node*>(index.parent().internalPointer())), 1, parent);
    return parent;
}
void SceneModel::removeNode(const QModelIndex &parent, const QModelIndex &index)
{
    FindPositionVisitor fpv(static_cast<vsg::Node*>(index.internalPointer()));
    removeRows(fpv(static_cast<vsg::Node*>(index.parent().internalPointer())), 1, parent);
}

QModelIndex SceneModel::index(const vsg::Node *node) const
{
    ParentVisitor parentVisitor(node);
    root->accept(parentVisitor);
    if (parentVisitor.pathToChild.empty() || parentVisitor.pathToChild.size() < 2)
        return QModelIndex();
    auto parent = *(parentVisitor.pathToChild.end() - 2);

    FindPositionVisitor fpv(node);
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
    auto parentNode = static_cast<vsg::Node*>(parent.internalPointer());

    int rows = 0;

    auto autoF = [&rows](const auto& node) { rows = node.children.size(); };
    //auto trajF = [&rows](const SceneTrajectory& node) { rows = node.traj->size(); };

    CFunctionVisitor<decltype (autoF)> fv(autoF, autoF);
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
            if(auto sw = static_cast<vsg::Node*>(index.parent().internalPointer())->cast<vsg::Switch>(); sw)
                return sw->children.at(index.row()).enabled ? Qt::Checked : Qt::Unchecked;
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
            if(auto rail = nodeInfo->cast<TrackSection>(); rail != nullptr)
                return rail->inclination;
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

        CFunctionVisitor<decltype (autoF)> fv(autoF, autoF);
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
        auto visit = [index, value](vsg::Switch& node)
        {
            node.children.at(index.row()).enabled = value.toBool();
        };
        Lambda1Visitor<decltype (visit), vsg::Switch> lv(visit);
        static_cast<vsg::Node*>(index.parent().internalPointer())->accept(lv);
        return true;
    }
    if (role != Qt::EditRole)
        return false;
    if (index.column() == Name)
    {
        QString newName = value.toString();
        QUndoCommand *command = new RenameObject(nodeInfo, newName);

        Q_ASSERT(undoStack != Q_NULLPTR);
        undoStack->push(command);

        emit dataChanged(index, index.sibling(index.row(), ColumnCount));
        return true;
    }
    else if (index.column() == Option)
    {
            if(auto rail = nodeInfo->cast<TrackSection>(); rail != nullptr)
            {
                Q_ASSERT(undoStack != Q_NULLPTR);
                undoStack->push(new ChangeIncl(rail, value.toDouble()));
                return true;
            }
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
        case Option:
        {
            if(static_cast<vsg::Node*>(index.internalPointer())->is_compatible(typeid (TrackSection)))
                flags |= Qt::ItemIsEditable;
            break;
        }
        }
    }
    return flags;
}
