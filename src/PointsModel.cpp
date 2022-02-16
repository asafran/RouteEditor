#include "PointsModel.h"
#include "topology.h"
#include <QFont>
#include <QSize>

PointsModel::PointsModel(route::SplineTrajectory *trj, QUndoStack *stack, QObject *parent) :
    QAbstractTableModel(parent)
  , _trajectory(trj)
  , _undoStack(stack)
{
}

PointsModel::~PointsModel()
{}

QVariant PointsModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    int row = index.row() - 1;
    int col = index.column();
    Q_ASSERT(col < ColumnCount || row <= _trajectory->_points.size());

    switch (role) {
    case Qt::FontRole:
        break;
    case Qt::BackgroundRole:
        break;
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::SizeHintRole:
        return QSize(10, 20);
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        vsg::ref_ptr<route::RailPoint> rp;
        if(row == -1)
            rp = _trajectory->_fwdPoint;
        else if(row == _trajectory->_points.size())
            rp = _trajectory->_bwdPoint;
        else
            rp = _trajectory->_points.at(row);
        switch (col) {
        case Incliantion:
            return 0.0;
        case Tangent:
            return rp->_tangent;
        case Tilt:
            return rp->_tilt;
        case CatenaryHeight:
            return rp->_cheight;
        }
    }
    default:
        break;
    }
    return QVariant();
}
QVariant PointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case Incliantion:
            return QObject::tr("Уклон");
        case Tangent:
            return QObject::tr("Вес");
        case Tilt:
            return QObject::tr("Наклон");
        case CatenaryHeight:
            return QObject::tr("Высота КС");
        }
    }
    return QVariant();
}
Qt::ItemFlags PointsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags _flags = QAbstractTableModel::flags(index);
    int row = index.row();
    int col = index.column();
    Q_ASSERT(col < ColumnCount || row <= _trajectory->_points.size());

    return _flags |= Qt::ItemIsEditable | Qt::ItemIsSelectable;
}
/*
bool PointsModel::setData(const QModelIndex & modelindex, const QVariant & value, int role)
{
    int row = modelindex.row();
    int col = modelindex.column();
    Q_ASSERT(col < ColumnCount || row <= _trajectory->_points.size());

    if(role == Qt::EditRole && selectedObject)
    {
        if(col == 1)
        {
            if(row == NAME)
            {
              QString newName = value.toString();
              undoStack->push(new RenameObject(selectedObject, newName));
              return true;
            } else if(row >= COORD_ECEFX && row <= COORD_ECEFZ)
            {
                auto newpos = selectedObject->position;
                newpos[row-COORD_ECEFX] = value.toDouble();
                undoStack->push(new MoveObject(selectedObject, newpos));
                return true;
            } else if(row >= COORDX && row <= COORDZ)
            {
                auto lla = ellipsoidModel->convertECEFToLatLongAltitude(selectedObject->position);
                lla[row-COORDX] = value.toDouble();
                undoStack->push(new MoveObject(selectedObject, ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
                return true;
            } else
                switch (row) {
                case ROTATEX:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dquat(qDegreesToRadians(value.toDouble() - data(index(ROTATEX,1)).toDouble()), vsg::dvec3(1.0, 0.0, 0.0))));
                    return true;
                }
                case ROTATEY:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dquat(qDegreesToRadians(value.toDouble() - data(index(ROTATEY,1)).toDouble()), vsg::dvec3(0.0, 1.0, 0.0))));
                    return true;
                }
                case ROTATEZ:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dquat(qDegreesToRadians(value.toDouble() - data(index(ROTATEZ,1)).toDouble()), vsg::dvec3(0.0, 0.0, 1.0))));
                    return true;
                }
                default:
                    break;
                }
        }
    }
    return false;
}
*/
int PointsModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return ColumnCount;
}
int PointsModel::rowCount ( const QModelIndex & /*parent*/) const
{
    return _trajectory->_points.size() + 2;
}
/*
void PointsModel::setTraj(route::SplineTrajectory *trj)
{
    auto rows = rowCount();
    _trajectory = trj;
    //emit dataChanged(index(0,0), index(rows, columnCount()));
}*/


