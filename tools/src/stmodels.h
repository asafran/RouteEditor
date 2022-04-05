#ifndef ROUTESMODEL_H
#define ROUTESMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include "topology.h"

class RouteBeginModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteBeginModel(QObject *parent = nullptr, route::Station* st = nullptr);

    ~RouteBeginModel();

    void setStation(route::Station* st);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<route::Station> _st;
};

class RouteEndModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteEndModel(QObject *parent = nullptr, route::Routes* r = nullptr);

    ~RouteEndModel();

    void setRoutes(route::Routes* r);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<route::Routes> _r;
};

class RouteCmdModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteCmdModel(QObject *parent = nullptr, route::Route* r = nullptr);

    ~RouteCmdModel();

    void setRoute(route::Route* r);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    bool insertCmd(vsg::ref_ptr<route::Command> cmd);
    bool removeRows(int row, int count, const QModelIndex &parent);

private:
    vsg::ref_ptr<route::Route> _r;
};

class StationsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit StationsModel(vsg::ref_ptr<route::Topology> topo, QObject *parent = nullptr);

    ~StationsModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    route::Station* station(const QModelIndex &index) const;

    //void fetchMore(const QModelIndex &parent);
    //bool canFetchMore(const QModelIndex &parent) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

    bool insertRows(int row, int count, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);

private:
    vsg::ref_ptr<route::Topology> _topology;
};

#endif // ROUTESMODEL_H
