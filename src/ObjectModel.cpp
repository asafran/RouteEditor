#include "ObjectModel.h"
#include "ParentVisitor.h"
#include <vsg/maths/transform.h>
#include <QFont>
#include "undo-redo.h"

ObjectModel::ObjectModel(vsg::ref_ptr<vsg::EllipsoidModel> ellipsoid, QUndoStack *stack, QObject *parent) :
    QAbstractTableModel(parent)
  , ellipsoidModel(ellipsoid)
  , undoStack(stack)
{

}
/*
ObjectModel::~ObjectModel()
{}
*/
QVariant ObjectModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();
    Q_ASSERT(col < 2 || row < ROW_COUNT);

    switch (role) {
    case Qt::FontRole:
        if (col == 0) {
            QFont boldFont;
            boldFont.setBold(true);
            return boldFont;
        }
        break;
    case Qt::BackgroundRole:
        break;
    case Qt::TextAlignmentRole:
        if (col == 0)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        else
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        break;
    }
    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if(col == 1)
            if(!selectedObject)
                return QVariant();
            else
                switch (row) {
                case NAME:
                {
                    std::string name;
                    if(selectedObject->getValue(META_NAME, name))
                        return name.c_str();
                    break;
                }
                case COORD_ECEFX:
                    return selectedObject->world()[3].x;
                case COORD_ECEFY:
                    return selectedObject->world()[3].y;
                case COORD_ECEFZ:
                    return selectedObject->world()[3].z;
                case COORDX:
                    return ellipsoidModel->convertECEFToLatLongAltitude(vsg::dvec3(selectedObject->world()[3].x, selectedObject->world()[3].y, selectedObject->world()[3].z)).x;
                case COORDY:
                    return ellipsoidModel->convertECEFToLatLongAltitude(vsg::dvec3(selectedObject->world()[3].x, selectedObject->world()[3].y, selectedObject->world()[3].z)).y;
                case COORDZ:
                    return ellipsoidModel->convertECEFToLatLongAltitude(vsg::dvec3(selectedObject->world()[3].x, selectedObject->world()[3].y, selectedObject->world()[3].z)).z;
                case ROTATEX:
                {
                    double sinr_cosp = 2 * (selectedObject->quat.w * selectedObject->quat.x + selectedObject->quat.y * selectedObject->quat.z);
                    double cosr_cosp = 1 - 2 * (selectedObject->quat.x * selectedObject->quat.x + selectedObject->quat.y * selectedObject->quat.y);
                    return qRadiansToDegrees(std::atan2(sinr_cosp, cosr_cosp));
                }
                case ROTATEY:
                {
                    double sinp = 2 * (selectedObject->quat.w * selectedObject->quat.y - selectedObject->quat.z * selectedObject->quat.x);
                    if (std::abs(sinp) >= 1)
                        return qRadiansToDegrees(std::copysign(vsg::PI / 2, sinp)); // use 90 degrees if out of range
                    else
                        return qRadiansToDegrees(std::asin(sinp));
                }
                case ROTATEZ:
                {
                    double siny_cosp = 2 * (selectedObject->quat.w * selectedObject->quat.z + selectedObject->quat.x * selectedObject->quat.y);
                    double cosy_cosp = 1 - 2 * (selectedObject->quat.y * selectedObject->quat.y + selectedObject->quat.z * selectedObject->quat.z);
                    return qRadiansToDegrees(std::atan2(siny_cosp, cosy_cosp));
                }
                }
        else //if(col == 0)
            switch (row) {
            case NAME:
                return QObject::tr("Имя");
            case COORD_ECEFX:
                return QObject::tr("Координата Х по ECEF");
            case COORD_ECEFY:
                return QObject::tr("Координата Y по ECEF");
            case COORD_ECEFZ:
                return QObject::tr("Координата Z по ECEF");
            case COORDX:
                return QObject::tr("Координата X");
            case COORDY:
                return QObject::tr("Координата Y");
            case COORDZ:
                return QObject::tr("Координата Z");
            case ROTATEX:
                return QObject::tr("Вращение по X");
            case ROTATEY:
                return QObject::tr("Вращение по Y");
            case ROTATEZ:
                return QObject::tr("Вращение по Z");
            }
    }
    return QVariant();
}
QVariant ObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QObject::tr("Параметр");
        case 1:
            return QObject::tr("Значение");
        }
    }
    return QVariant();
}
Qt::ItemFlags ObjectModel::flags(const QModelIndex &index) const
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

bool ObjectModel::setData(const QModelIndex & modelindex, const QVariant & value, int role)
{
    int row = modelindex.row();
    int col = modelindex.column();
    Q_ASSERT(col < 2 || row < ROW_COUNT);
    if(role == Qt::EditRole && selectedObject)
    {
        auto worldToLocal = vsg::inverse(selectedObject->localToWord);
        if(col == 1)
        {
            if(row == NAME)
            {
              QString newName = value.toString();
              undoStack->push(new RenameObject(selectedObject, newName));
              return true;
            } else if(row >= COORD_ECEFX && row <= COORD_ECEFZ)
            {
                auto newmat = selectedObject->world();
                newmat[3][row-COORD_ECEFX] += (value.toDouble() - selectedObject->world()[3][row-COORD_ECEFX]);
                undoStack->push(new MoveObject(selectedObject, newmat));
                return true;
            } else if(row >= COORDX && row <= COORDZ)
            {
                auto lla = ellipsoidModel->convertECEFToLatLongAltitude(vsg::dvec3(selectedObject->world()[3].x, selectedObject->world()[3].y, selectedObject->world()[3].z));
                lla[row-COORDX] = value.toDouble();
                auto newpos = vsg::translate(ellipsoidModel->convertLatLongAltitudeToECEF(lla));
                undoStack->push(new MoveObject(selectedObject, newpos * worldToLocal));
                return true;
            } else
                switch (row) {
                case ROTATEX:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dvec3(1.0, 0.0, 0.0), qDegreesToRadians(value.toDouble() - data(index(ROTATEX,1)).toDouble())));
                    return true;
                }
                case ROTATEY:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dvec3(0.0, 1.0, 0.0), qDegreesToRadians(value.toDouble() - data(index(ROTATEY,1)).toDouble())));
                    return true;
                }
                case ROTATEZ:
                {
                    undoStack->push(new RotateObject(selectedObject, vsg::dvec3(0.0, 0.0, 1.0), qDegreesToRadians(value.toDouble() - data(index(ROTATEZ,1)).toDouble())));
                    return true;
                }
                default:
                    break;
                }
        }
    }
    return false;
}

int ObjectModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return 2;
}
int ObjectModel::rowCount ( const QModelIndex & parent) const
{
    return ROW_COUNT;
}

void ObjectModel::selectObject(const QItemSelection &selected, const QItemSelection &deselected)
{
    if(auto object = static_cast<vsg::Node*>(selected.indexes().front().internalPointer())->cast<SceneObject>(); object)
    {
        selectedObject = object;
        emit dataChanged(index(0,0), index(ROW_COUNT, 1));
    }

}

