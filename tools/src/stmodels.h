#ifndef ROUTESMODEL_H
#define ROUTESMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include "topology.h"

class RouteBeginModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteBeginModel(vsg::ref_ptr<route::Station> st, QObject *parent = nullptr);

    ~RouteBeginModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<route::Station> _st;
};

class RouteEndModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteEndModel(vsg::ref_ptr<route::Routes> r, QObject *parent = nullptr);

    ~RouteEndModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    vsg::ref_ptr<route::Routes> _r;
};

class RouteCmdModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RouteCmdModel(vsg::ref_ptr<route::Route> r, QObject *parent = nullptr);

    ~RouteCmdModel();

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

class StagesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit StagesModel(QObject *parent = nullptr);

    ~StagesModel();

    void setStation(route::Station* station);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    //bool insertStage(route::Station* to, vsg::ref_ptr<route::Stage> stg);
    void reset();
    bool removeRows(int row, int count, const QModelIndex &parent);

private:
    vsg::ref_ptr<route::Station> _station;
};

#endif // ROUTESMODEL_H
