#include "stmodels.h"

RouteBeginModel::RouteBeginModel(vsg::ref_ptr<route::Station> st, QObject *parent)
    : QAbstractListModel{parent}
    , _st(st) {}

RouteBeginModel::~RouteBeginModel() {}

int RouteBeginModel::rowCount(const QModelIndex &parent) const
{
    return _st->rsignals.size();
}

QVariant RouteBeginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !(role == Qt::DisplayRole && role == Qt::EditRole))
        return QVariant();
    Q_ASSERT(index.row() < _st->rsignals.size());
    auto it = std::next(_st->rsignals.cbegin(), index.row());
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
    return _r->routes.size();
}

QVariant RouteEndModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !(role == Qt::DisplayRole && role == Qt::EditRole))
        return QVariant();
    Q_ASSERT(index.row() < _r->routes.size());
    auto it = std::next(_r->routes.cbegin(), index.row());
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
    if (!index.isValid() || !(role == Qt::DisplayRole || role == Qt::EditRole))
        return QVariant();
    Q_ASSERT(index.row() < _topology->stations.size());
    auto it = std::next(_topology->stations.cbegin(), index.row());
    return it->first.c_str();
}

route::Station *StationsModel::station(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    Q_ASSERT(index.row() < _topology->stations.size());
    auto it = std::next(_topology->stations.cbegin(), index.row());
    return it->second;
}

/*
void StationsModel::fetchMore(const QModelIndex &parent)
{
    auto rows = rowCount();
    beginInsertRows(QModelIndex(), rows - 1, rows - 1);
    endInsertRows();
    _fetched = rows;
    emit dataChanged(index(0), index(rows));
}

bool StationsModel::canFetchMore(const QModelIndex &parent) const
{
    return _fetched != rowCount();
}
*/

bool StationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid() || index.row() > rowCount() || role != Qt::EditRole)
        return false;
    Q_ASSERT(index.row() < _topology->stations.size());
    auto it = std::next(_topology->stations.begin(), index.row());
    auto station = it->second;
    if(_topology->stations.insert({value.toString().toStdString(), station}).second)
    {
        beginResetModel();
        _topology->stations.erase(it);
        endResetModel();
        return true;
    }
    return false;
}

bool StationsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    bool found = _topology->stations.find("НоваяСтанция") != _topology->stations.end();
    if(count != 1 || row != rowCount() || found)
        return false;
    beginResetModel();
    _topology->stations.insert({"НоваяСтанция", route::Station::create()});
    endResetModel();
    return true;
}

bool StationsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(count != 1)
        return false;
    beginRemoveRows(QModelIndex(), row, 1);
    _topology->stations.erase(std::next(_topology->stations.begin(), row));
    endRemoveRows();
    emit dataChanged(index(0), index(rowCount()));
    return true;
}

RouteCmdModel::RouteCmdModel(vsg::ref_ptr<route::Route> r, QObject *parent)
    : QAbstractListModel{parent}
    , _r(r) {}

RouteCmdModel::~RouteCmdModel() {}

int RouteCmdModel::rowCount(const QModelIndex &parent) const
{
    return _r->commands.size();
}

bool RouteCmdModel::insertCmd(vsg::ref_ptr<route::Command> cmd)
{
    if(!cmd)
        return false;
    int row = rowCount(QModelIndex());

    beginInsertRows(QModelIndex(), row, row);
    _r->commands.push_back(cmd);
    endInsertRows();
    return true;
}

bool RouteCmdModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(count == 0 || row >= _r->commands.size())
        return false;
    auto it = std::next(_r->commands.begin(), row);
    beginRemoveRows(QModelIndex(), row, 1);
    if(count == 1)
        _r->commands.erase(it);
    else
        _r->commands.erase(it, it + count - 1);
    endRemoveRows();
    return true;
}

QVariant RouteCmdModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !(role == Qt::DisplayRole && role == Qt::EditRole))
        return QVariant();
    auto cmd = _r->commands.at(index.row());
    if(auto sig = cmd.cast<route::SignalCommand>(); sig)
    {
        std::string name;
        if(sig->_sig->getValue(app::NAME, name))
            return name.c_str();
        return tr("Без литеры");
    } else if (auto jct = cmd.cast<route::JunctionCommand>(); jct)
    {
        std::string name;
        if(jct->_j->getValue(app::NAME, name))
            return name.c_str();
        return tr("Без имени");
    }
    return QVariant();
}

StagesModel::StagesModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

StagesModel::~StagesModel()
{

}

void StagesModel::setStation(route::Station *station)
{
    beginResetModel();
    _station = station;
    endResetModel();
}

int StagesModel::rowCount(const QModelIndex &parent) const
{
    return _station->stages.size();
}

QVariant StagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !(role == Qt::DisplayRole || role == Qt::EditRole))
        return QVariant();
    Q_ASSERT(index.row() < _station->stages.size());
    auto it = std::next(_station->stages.cbegin(), index.row());

    std::string begin;
    _station->getValue(app::NAME, begin);
    std::string end;
    it->first->getValue(app::NAME, end);
    return (begin + " ===>>> " + end).c_str();
}

void StagesModel::reset()
{
    beginResetModel();
    endResetModel();
}
/*
bool StagesModel::insertStage(route::Station* to, vsg::ref_ptr<route::Stage> stg)
{
    if(_station->stages.find(to) != _station->stages.end())
        return false;
    beginResetModel();
    _station->stages.insert({to, stg});
    endResetModel();
    return true;
}
*/
bool StagesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(count != 1)
        return false;
    beginRemoveRows(QModelIndex(), row, 1);
    _station->stages.erase(std::next(_station->stages.begin(), row));
    endRemoveRows();
    emit dataChanged(index(0), index(rowCount()));
    return true;
}
