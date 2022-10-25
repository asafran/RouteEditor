#ifndef UNDO_H
#define UNDO_H

#include "SceneModel.h"
#include "topology.h"
#include "DatabaseManager.h"

class AddSceneObject : public QUndoCommand
{
public:
    AddSceneObject(SceneModel *model,
            const QModelIndex &group,
            vsg::ref_ptr<vsg::Node> node,
            QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _model(model)
        , _group(group)
        , _node(node)
    {
        std::string name;
        node->getValue(app::NAME, name);
        if(name.empty())
            name = node->className();
        setText(QObject::tr("Новый объект %1").arg(name.c_str()));
    }
    AddSceneObject(SceneModel *model,
            vsg::Node *group,
            vsg::ref_ptr<vsg::Node> node,
            QUndoCommand *parent = nullptr)
        : AddSceneObject(model, model->index(group), node, parent)
    {
    }
    void undo() override
    {
        _model->removeNode(_model->index(_row, 0, _group));
        if(auto trj = _node.cast<route::Trajectory>(); trj)
            trj->detatch();
    }
    void redo() override
    {
        _row = _model->addNode(_group, _node);
        if(auto trj = _node.cast<route::Trajectory>(); trj)
            trj->attach();
    }
private:
    SceneModel *_model;
    int _row;
    const QModelIndex _group;
    vsg::ref_ptr<vsg::Node> _node;

};

class AddSignal : public QUndoCommand
{
public:
    AddSignal(route::RailConnector *rc,
              vsg::ref_ptr<signalling::Signal> sig,
              vsg::ref_ptr<route::Topology> topo,
              bool connect,
              QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _rc(rc)
        , _sig(sig)
        , _topo(topo)
        , _connect(connect) {}

    void undo() override
    {
        if(_sig->station.empty())
            return;
        try {
            auto& sigs = _topo->stations.at(_sig->station)->rsignals;
            _routes = sigs.at(_sig.get());
            sigs.erase(_sig.get());
        }  catch (std::out_of_range) {}
    }
    void redo() override
    {
        if(_sig->station.empty())
            return;
        try {
            auto& sigs = _topo->stations.at(_sig->station)->rsignals;
            if(!_routes)
                _routes = signalling::Routes::create();
            sigs.insert({_sig, _routes});
        }  catch (std::out_of_range) {}
    }
protected:
    vsg::ref_ptr<route::RailConnector> _rc;
    vsg::ref_ptr<signalling::Signal> _sig;
    vsg::ref_ptr<signalling::Routes> _routes;
    vsg::ref_ptr<route::Topology> _topo;

    bool _connect;
};

class AddFwdSignal : public AddSignal
{
public:
    AddFwdSignal(route::RailConnector *rc,
                 vsg::ref_ptr<signalling::Signal> sig,
                 vsg::ref_ptr<route::Topology> topo,
                 bool connect,
                 QUndoCommand *parent = nullptr)
        : AddSignal(rc, sig, topo, connect,parent)
    {
        setText(QObject::tr("Добавлен сигнал, направление вперед"));
    }

    void undo() override
    {
        _rc->setSignal(vsg::ref_ptr<signalling::Signal>(), _connect);
        AddSignal::undo();
    }
    void redo() override
    {
        _rc->setSignal(_sig, _connect);
        AddSignal::redo();
    }
};

class AddBwdSignal : public AddSignal
{
public:
    AddBwdSignal(route::RailConnector *rc,
                 vsg::ref_ptr<signalling::Signal> sig,
                 vsg::ref_ptr<route::Topology> topo,
                 bool connect,
                 QUndoCommand *parent = nullptr)
        : AddSignal(rc, sig, topo, connect, parent)
    {
        setText(QObject::tr("Добавлен сигнал, направление назад"));
    }

    void undo() override
    {
        _rc->setReverseSignal(vsg::ref_ptr<signalling::Signal>(), _connect);
        AddSignal::undo();
    }
    void redo() override
    {
        _rc->setReverseSignal(_sig, _connect);
        AddSignal::redo();
    }

};
class RemoveBwdSignal : public AddBwdSignal
{
public:
    RemoveBwdSignal(route::RailConnector *rc,
                    vsg::ref_ptr<route::Topology> topo,
                    QUndoCommand *parent = nullptr)
        : AddBwdSignal(rc, rc->bwdSignal(), topo, rc->bwdConnected, parent)
    {
        setText(QObject::tr("Удален сигнал, направление назад"));
    }

    void undo() override
    {
        AddBwdSignal::redo();
    }
    void redo() override
    {
        AddBwdSignal::undo();
    }
};

class RemoveFwdSignal : public AddFwdSignal
{
public:
    RemoveFwdSignal(route::RailConnector *rc,
                    vsg::ref_ptr<route::Topology> topo,
                    QUndoCommand *parent = nullptr)
        : AddFwdSignal(rc, rc->fwdSignal(), topo, rc->fwdConnected, parent)
    {
        setText(QObject::tr("Удален сигнал, направление вперед"));
    }

    void undo() override
    {
        AddFwdSignal::redo();
    }
    void redo() override
    {
        AddFwdSignal::undo();
    }
};

class RemoveNode : public QUndoCommand
{
public:
    RemoveNode(SceneModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _model(model)
        , _node(static_cast<vsg::Node*>(index.internalPointer()))
        , _group(index.parent())
        , _row(index.row())
    {
        std::string name;
        _node->getValue(app::NAME, name);
        if(name.empty())
            name = _node->className();
        setText(QObject::tr("Удален объект %1").arg(name.c_str()));
        if(auto scobj = _node.cast<route::SceneObject>(); scobj)
            scobj->setSelection(false);
    }
    void undo() override
    {
        _row = _model->addNode(_group, _node);
        if(auto trj = _node.cast<route::Trajectory>(); trj)
            trj->attach();
    }
    void redo() override
    {
        _model->removeNode(_model->index(_row, 0, _group));
        if(auto trj = _node.cast<route::Trajectory>(); trj)
            trj->detatch();
    }
private:
    SceneModel *_model;
    int _row;
    vsg::ref_ptr<vsg::Node> _node;
    const QModelIndex _group;

};

class RenameObject : public QUndoCommand
{
public:
    RenameObject(vsg::Object *obj, const QString &name, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(obj)
        , _newName(name.toStdString())
    {
        obj->getValue(app::NAME, _oldName);
        setText(QObject::tr("Oбъект %1, новое имя %2").arg(_oldName.c_str()).arg(name));
    }
    void undo() override
    {
        _object->setValue(app::NAME, _oldName);
    }
    void redo() override
    {
        _object->setValue(app::NAME, _newName);
    }
    int id() const override
    {
        return 10;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const RenameObject*>(other);
        if(rcmd->_object != _object)
            return false;
        _newName = rcmd->_newName;
        return true;
    }
private:
    vsg::ref_ptr<vsg::Object> _object;
    std::string _oldName;
    std::string _newName;

};
/*
class ChangeIncl : public QUndoCommand
{
public:
    ChangeIncl(TrackSection *rail, double incl, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _rail(rail)
        , _oldIncl(rail->inclination)
        , _newIncl(incl)
    {
        setText(QObject::tr("Изменен уклон %1 на %2").arg(QString::number(_oldIncl)).arg(QString::number(_newIncl)));
    }
    void undo() override
    {
        _rail->inclination = _oldIncl;
        _rail->traj->recalculatePositions();
    }
    void redo() override
    {
        _rail->inclination = _newIncl;
        _rail->traj->recalculatePositions();
    }
private:
    TrackSection *_rail;
    const double _oldIncl;
    const double _newIncl;

};
*/
class RotateObject : public QUndoCommand
{
public:
    RotateObject(route::SceneObject *object, vsg::dquat q, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _oldQ(object->getRotation())
        , _newQ(q)
    {
        std::string name;
        _object->getValue(app::NAME, name);
        setText(QObject::tr("Повернут объект %1").arg(name.c_str()));
    }
    void undo() override
    {
        _object->setRotation(_oldQ);
    }
    void redo() override
    {
        _object->setRotation(_newQ);
    }
    int id() const override
    {
        return 2;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const RotateObject*>(other);
        if(rcmd->_object != _object)
            return false;
        _newQ = rcmd->_newQ;
        return true;
    }
private:
    vsg::ref_ptr<route::SceneObject> _object;
    const vsg::dquat _oldQ;
    vsg::dquat _newQ;
};

class MoveObject : public QUndoCommand
{
public:
    MoveObject(route::SceneObject *object, const vsg::dvec3& pos, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _oldPos(object->getPosition())
        , _newPos(pos)
    {
        std::string name;
        object->getValue(app::NAME, name);
        setText(QObject::tr("Перемещен объект %1, ECEF %2").arg(name.c_str())
                .arg("X=" + QString::number(pos.x) + " Y=" + QString::number(pos.x) + " Z=" + QString::number(pos.x)));

    }
    void undo() override
    {
        _object->setPosition(_oldPos);
    }
    void redo() override
    {
        _object->setPosition(_newPos);
    }
    int id() const override
    {
        return 1;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto mcmd = static_cast<const MoveObject*>(other);
        if(mcmd->_object != _object)
            return false;
        _newPos = mcmd->_newPos;
        return true;
    }

protected:
    vsg::ref_ptr<route::SceneObject> _object;
    const vsg::dvec3 _oldPos;
    vsg::dvec3 _newPos;

};

class MoveObjectOnTraj : public QUndoCommand
{
public:
    MoveObjectOnTraj(vsg::MatrixTransform *object, double coord, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _newPos(coord)
    {
        std::string name;
        object->getValue(app::NAME, name);
        setText(QObject::tr("Перемещен объект %1, путевая коодината %2").arg(name.c_str()).arg(QString::number(coord)));

        if(!object->getValue(app::PROP, _oldPos))
            _oldPos = 0.0;

        vsg::Node *parentNode = nullptr;
        object->getValue(app::PARENT, parentNode);
        _parent = parentNode->cast<route::SplineTrajectory>();
    }
    void undo() override
    {
        _object->matrix = _parent->getMatrixAt(_oldPos);
        _object->setValue(app::PROP, _oldPos);

        ApplyTransform ct;
        ct.stack.push(vsg::dmat4());
        _parent->accept(ct);
    }
    void redo() override
    {
        _object->matrix = _parent->getMatrixAt(_newPos);
        _object->setValue(app::PROP, _newPos);

        ApplyTransform ct;
        ct.stack.push(vsg::dmat4());
        _parent->accept(ct);
    }
    int id() const override
    {
        return 3;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto mcmd = static_cast<const MoveObjectOnTraj*>(other);
        if(mcmd->_object != _object || mcmd->_parent != _parent)
            return false;
        _newPos = mcmd->_newPos;
        return true;
    }

protected:
    vsg::ref_ptr<route::SplineTrajectory> _parent;
    vsg::ref_ptr<vsg::MatrixTransform> _object;
    double _oldPos;
    double _newPos;
};

class ConnectRails : public QUndoCommand
{
public:
    ConnectRails(route::RailConnector *conn2, route::SplineTrajectory *connectable, bool front, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _conn2(conn2)
        , _traj(connectable)
        , _setfront(front)
    {
        setText(QObject::tr("Соединены траектории"));

        _conn1 = _setfront ? _traj->getFwdPoint() : _traj->getBwdPoint();
    }
    void undo() override
    {
        _setfront ? _traj->setFwdPoint(_conn1) : _traj->setBwdPoint(_conn1);
        _traj->recalculate();
    }
    void redo() override
    {
        _setfront ? _traj->setFwdPoint(_conn2) : _traj->setBwdPoint(_conn2);
        _traj->recalculate();
    }
private:
    vsg::ref_ptr<route::RailConnector> _conn1;
    vsg::ref_ptr<route::RailConnector> _conn2;
    bool _setfront;
    vsg::ref_ptr<route::SplineTrajectory> _traj;
};

class AddRailPoint : public QUndoCommand
{
public:
    AddRailPoint(route::SplineTrajectory *trajectory, vsg::ref_ptr<route::RailPoint> point, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _trajectory(trajectory)
        , _point(point)
    {
        std::string name;
        trajectory->getValue(app::NAME, name);
        setText(QObject::tr("Добавлена точка в траекторию %1").arg(name.c_str()));
    }
    void undo() override
    {
        _trajectory->remove(_point);
    }
    void redo() override
    {
        _trajectory->add(_point);
    }
private:
    vsg::ref_ptr<route::SplineTrajectory> _trajectory;
    vsg::ref_ptr<route::RailPoint> _point;
};

class RemoveRailPoint : public QUndoCommand
{
public:
    RemoveRailPoint(route::SplineTrajectory *trajectory, route::RailPoint *point, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _trajectory(trajectory)
        , _point(point)
    {
        std::string name;
        trajectory->getValue(app::NAME, name);
        setText(QObject::tr("Добавлена точка в траекторию %1").arg(name.c_str()));
    }
    void undo() override
    {
        _trajectory->add(_point);
    }
    void redo() override
    {
        _trajectory->remove(_point);
    }
private:
    vsg::ref_ptr<route::SplineTrajectory> _trajectory;
    vsg::ref_ptr<route::RailPoint> _point;
};

class ApplyTransformCalculation : public MoveObject
{
public:
    ApplyTransformCalculation(route::SceneObject *object, const vsg::dvec3& pos, const vsg::dmat4& ltw, QUndoCommand *parent = nullptr)
        : MoveObject(object, pos, parent)
        , _oldLtw(object->localToWorld)
        , _newLtw(ltw)
    {
        std::string name;
        object->getValue(app::NAME, name);
        setText(QObject::tr("Перемещен объект %1, ECEF %2").arg(name.c_str())
                .arg("X=" + QString::number(pos.x) + " Y=" + QString::number(pos.x) + " Z=" + QString::number(pos.x)));

    }
    void undo() override
    {
        MoveObject::undo();
        _object->localToWorld = _oldLtw;
    }
    void redo() override
    {
        MoveObject::redo();
        _object->localToWorld = _newLtw;
    }
    int id() const override
    {
        return QUndoCommand::id();
    }

private:
    const vsg::dmat4 _oldLtw;
    const vsg::dmat4 _newLtw;
};

template<typename F, typename V>
class ExecuteLambda : public QUndoCommand
{
public:
    ExecuteLambda(F func, V old, V val, int id, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _newProp(val)
        , _oldProp(old)
        , _func(func)
        , _id(id)
    {
    }
    void undo() override
    {
        _func(_oldProp);
    }
    void redo() override
    {
        _func(_newProp);
    }
    int id() const override
    {
        return _id;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto excmd = static_cast<const ExecuteLambda<F,V>*>(other);
        _newProp = excmd->_newProp;
        return true;
    }

protected:
    F _func;
    int _id;

    const V _oldProp;
    V _newProp;
};
/*
class SetRailProp : public ExecuteLambda<std::function<void(double)>, double>
{
public:
    SetRailProp(std::function<void(double)> func, double old, double val, QUndoCommand *parent = nullptr)
        : ExecuteLambda(func, old, val, parent)
    {
        setText(QObject::tr("Изменены параметры точки"));
    }
};


class DeltaMoveObject : public MoveObject
{
public:
    DeltaMoveObject(route::SceneObject *object, const vsg::dvec3& delta, QUndoCommand *parent = nullptr)
        : MoveObject(object, object->getPosition() + delta)
    {}
};

class AddTrajectory : public QUndoCommand
{
public:
    AddTrajectory(route::Topology *topology, std::string name, Trajectory *prev = nullptr, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _topo(topology)
        , _prev(prev)
        , _traj(Trajectory::create(name))
    {
        setText(QObject::tr("Добавлена траектория %1").arg(name.c_str()));
    }
    void undo() override
    {
        if(_traj->bwdTraj != nullptr)
            _traj->bwdTraj = nullptr;
        if(_traj->fwdTraj != nullptr)
            _traj->fwdTraj = nullptr;
        _topo->trajectories.erase(it);
    }
    void redo() override
    {
        if(_prev != nullptr)
        {
            _prev->fwdTraj = _traj;
            _traj->bwdTraj = _prev;
        }
        it = _topo->insertTraj(_traj);
    }
    const vsg::ref_ptr<Trajectory> _traj;

private:
    Topology *_topo;
    Trajectory *_prev;
    Trajectories::iterator it;
};

class RemoveTrajectory : public QUndoCommand
{
public:
    RemoveTrajectory(Topology *topology, Trajectory *traj, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _topo(topology)
        , _prev(traj->bwdTraj)
    {
        std::string name;
        traj->getValue(app::NAME, name);
        setText(QObject::tr("Удалена траектория %1").arg(name.c_str()));
        it = _topo->trajectories.find(name);
        Q_ASSERT(it != _topo->trajectories.end());
    }
    void undo() override
    {
        std::string name;
        _traj->getValue(app::NAME, name);
        if(_prev != nullptr)
        {
            _prev->fwdTraj = _traj;
            _traj->bwdTraj = _prev;
        }
        _topo->insertTraj(_traj);
    }
    void redo() override
    {
        if(_traj->bwdTraj != nullptr)
            _traj->bwdTraj = nullptr;
        if(_traj->fwdTraj != nullptr)
            _traj->fwdTraj = nullptr;
        _topo->trajectories.erase(it);
    }
private:
    Topology *_topo;
    Trajectory *_prev;
    vsg::ref_ptr<Trajectory> _traj;
    Trajectories::iterator it;
};
*/
/*
class MoveTilePoint : public QUndoCommand
{
public:
    MoveTilePoint(const vsg::dvec3& oldpos, const vsg::dvec3& newpos, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _transform(transform)
        , _oldMat(transform->matrix)
    {
        std::string name;
        transform->getValue(app::NAME, name);
        setText(QObject::tr("Перемещен объект %1").arg(name.c_str()));

        _newMat = transform->matrix;
        _newMat[3][0] = pos[0];
        _newMat[3][1] = pos[1];
        _newMat[3][2] = pos[2];
    }
    void undo() override
    {
        _transform->matrix = _oldMat;
    }
    void redo() override
    {
        _transform->matrix = _newMat;
    }
private:
    vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBufferCmd;
    vsg::MatrixTransform *moving;

};
*/
#endif // UNDO_H
