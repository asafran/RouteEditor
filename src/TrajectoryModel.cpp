#include "TrajectoryModel.h"
#include "ParentVisitor.h"
#include <vsg/maths/transform.h>
#include <QFont>
#include "undo-redo.h"

TrajectoryModel::TrajectoryModel(QUndoStack *stack, QObject *parent) :
    QAbstractTableModel(parent)
  , undoStack(stack)
{}

TrajectoryModel::~TrajectoryModel()
{}

QVariant TrajectoryModel::data(const QModelIndex &index, int role) const
{

}
QVariant TrajectoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{

}
Qt::ItemFlags TrajectoryModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags _flags = QAbstractTableModel::flags(index);
    int row = index.row();
    int col = index.column();
    Q_ASSERT(col < 2 || row < ROW_COUNT);
    if(col == 0)
        return Qt::NoItemFlags;
    else
        return _flags |= Qt::ItemIsEditable;
}

bool TrajectoryModel::setData(const QModelIndex & modelindex, const QVariant & value, int role)
{
    int row = modelindex.row();
    int col = modelindex.column();
    Q_ASSERT(col < 2 || row < ROW_COUNT);
    if(role == Qt::EditRole && selectedObject)
    {

    }
    return false;
}

int TrajectoryModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return COLUMN_COUNT;
}
int TrajectoryModel::rowCount ( const QModelIndex & parent) const
{
    return ;
}

void TrajectoryModel::selectObject(const QModelIndex &modelindex)
{
    if(auto object = static_cast<vsg::Node*>(modelindex.internalPointer())->cast<SceneObject>(); object)
    {
        selectedObject = object;
        emit dataChanged(index(0,0), index(ROW_COUNT, 1));
    } else {
        selectedObject = nullptr;
        emit dataChanged(index(0,0), index(ROW_COUNT, 1));
    }
}
/*
void ObjectModel::rotateVertical()
{
    undoStack->beginMacro(tr("Заданно вертикальное положение"));
    undoStack->push(new RotateObject(selectedObject, vsg::dquat(0.0, 0.0, 0.0, 1.0)));
    selectedObject->quat = vsg::dquat(0.0, 0.0, 0.0, 1.0);
    undoStack->push(new RotateObject(selectedObject, vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), vsg::normalize(selectedObject->position()))));
    undoStack->endMacro();
}
*/
