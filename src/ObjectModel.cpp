#include "ObjectModel.h"
#include "ParentVisitor.h"
#include <vsg/maths/transform.h>

ObjectModel::ObjectModel(vsg::ref_ptr<vsg::EllipsoidModel> ellipsoid, QObject *parent) :
    QAbstractTableModel(parent)
  , ellipsoidModel(ellipsoid)
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
                return std::acos(1 - selectedObject->world()[0][0]);
            case ROTATEY:
                return std::acos(1 - selectedObject->world()[1][1]);
            case ROTATEZ:
                return std::acos(1 - selectedObject->world()[2][2]);
            case PARENT:
            {
                std::string name;
                if(selectedObject->parent->getValue(META_NAME, name))
                    return name.c_str();
                return selectedObject->parent->className();
            }
            }
    else if(col == 0)
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
        case PARENT:
            return QObject::tr("Родитель");
        }

    /*switch (role) {
    case Qt::DisplayRole:
        if (col == 0)
        {

        }
    case Qt::FontRole:
        if (row == 0 && col == 0) { //change font only for cell(0,0)
            QFont boldFont;
            boldFont.setBold(true);
            return boldFont;
        }
        break;
    case Qt::BackgroundRole:
        if (row == 1 && col == 2)  //change background only for cell(1,2)
            return QBrush(Qt::red);
        break;
    case Qt::TextAlignmentRole:
        if (row == 1 && col == 1) //change text alignment only for cell(1,1)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        break;
    case Qt::CheckStateRole:
        if (row == 1 && col == 0) //add a checkbox to cell(1,0)
            return Qt::Checked;
        break;
    }*/
    return QVariant();
}
QVariant ObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QObject::tr("first");
        case 1:
            return QObject::tr("second");
        }
    }
    return QVariant();
}
Qt::ItemFlags ObjectModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEditable;
}

bool ObjectModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
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

void ObjectModel::selectObject(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    if(command == QItemSelectionModel::Select)
        if(auto object = static_cast<vsg::Node*>(index.internalPointer())->cast<SceneObject>(); object)
            selectedObject = object;
}

