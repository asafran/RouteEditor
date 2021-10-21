#include "ObjectModel.h"
#include "ParentVisitor.h"


ObjectModel::ObjectModel(QObject *parent) :
    QAbstractItemModel(parent)
{

}

ObjectModel::~ObjectModel()
{}


int ObjectModel::columnCount ( const QModelIndex & /*parent = QModelIndex()*/ ) const
{
    return 2;
}
int ObjectModel::rowCount ( const QModelIndex & parent) const
{
    return ROW_COUNT;
}

