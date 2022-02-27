#include "stmodels.h"

RouteBeginModel::RouteBeginModel(vsg::ref_ptr<route::Station> st, QObject *parent)
    : QAbstractListModel{parent}
    , _st(st) {}

RouteBeginModel::~RouteBeginModel() {}

int RouteBeginModel::rowCount(const QModelIndex &parent) const
{
    return _st->_signals.size();
}

QVariant RouteBeginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto it = std::next(_st->_signals.cbegin(), index.row());
    auto sig = it->first;
    std::string name;
    if(sig->getValue(app::NAME, name))
        return name.c_str();
    return tr("Без литеры");
}

RouteEndModel::RouteEndModel(vsg::ref_ptr<route::Routes> r, QObject *parent)
    : QAbstractListModel{parent}
    , _r(r) {}

RouteEndModel::~RouteEndModel() {}

int RouteEndModel::rowCount(const QModelIndex &parent) const
{
    return _r->_routes.size();
}

QVariant RouteEndModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto it = std::next(_r->_routes.cbegin(), index.row());
    auto sig = it->first;
    std::string name;
    if(sig->getValue(app::NAME, name))
        return name.c_str();
    return tr("Без литеры");
}

StationsModel::StationsModel(vsg::ref_ptr<route::Topology> topo, QObject *parent)
    : QAbstractListModel{parent}
    , _topology(topo) {}

StationsModel::~StationsModel() {}

int StationsModel::rowCount(const QModelIndex &parent) const
{
    return _topology->stations.size();
}

QVariant StationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto it = std::next(_topology->stations.cbegin(), index.row());
    return it->first.c_str();
}


void StationsModel::fetchMore(const QModelIndex &parent)
{
    auto rows = rowCount();
    beginInsertRows(QModelIndex(), _fetched - 1, rows - 1);
    endInsertRows();
    _fetched = rows;
    emit dataChanged(index(0), index(rows));
}

bool StationsModel::canFetchMore(const QModelIndex &parent) const
{
    return _fetched != rowCount();
}


bool StationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid() || index.row() > rowCount())
        return false;
    auto it = std::next(_topology->stations.begin(), index.row());
    auto station = it->second;
    if(_topology->stations.insert({value.toString().toStdString(), station}).second)
    {
        _topology->stations.erase(it);
        emit dataChanged(StationsModel::index(0), StationsModel::index(rowCount()));
        return true;
    }
    return false;
}


bool StationsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    bool found = _topology->stations.find("НоваяСтанция") != _topology->stations.end();
    if(count != 1 || row != rowCount() || found)
        return false;
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _topology->stations.insert({"НоваяСтанция", route::Station::create()});
    endInsertRows();
    emit dataChanged(index(0), index(rowCount()));
    return true;
}

bool StationsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(count != 1)
        return false;
    beginRemoveRows(QModelIndex(), row, 1);
    _topology->stations.erase(std::next(_topology->stations.begin(), row));
    endInsertRows();
    emit dataChanged(index(0), index(rowCount()));
    return true;
}
