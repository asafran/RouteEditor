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
        node->getValue(META_NAME, name);
        if(name.empty())
            name = node->className();
        setText(QObject::tr("Новый объект %1").arg(name.c_str()));
    }
    void undo() override
    {
        _model->removeRows(_row, 1, _group);
    }
    void redo() override
    {
        _row = _model->addNode(_group, _node);
    }
private:
    SceneModel *_model;
    int _row;
    const QModelIndex _group;
    vsg::ref_ptr<vsg::Node> _node;

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
        _node->getValue(META_NAME, name);
        if(name.empty())
            name = _node->className();
        setText(QObject::tr("Удален объект %1").arg(name.c_str()));
        if(auto scobj = _node.cast<route::SceneObject>(); scobj)
            scobj->deselect();
    }
    void undo() override
    {
        _row = _model->addNode(_group, _node);
    }
    void redo() override
    {
        _model->removeRows(_row, 1, _group);
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
        , _obj(obj)
        , _newName(name.toStdString())
    {
        obj->getValue(META_NAME, _oldName);
        setText(QObject::tr("Oбъект %1, новое имя %2").arg(_oldName.c_str()).arg(name));
    }
    void undo() override
    {
        _obj->setValue(META_NAME, _oldName);
    }
    void redo() override
    {
        _obj->setValue(META_NAME, _newName);
    }
private:
    vsg::ref_ptr<vsg::Object> _obj;
    std::string _oldName;
    const std::string _newName;

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
        , _newQ(route::mult(object->getRotation(), q))
    {
        std::string name;
        _object->getValue(META_NAME, name);
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
        object->getValue(META_NAME, name);
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

class ApplyTransformCalculation : public MoveObject
{
public:
    ApplyTransformCalculation(route::SceneObject *object, const vsg::dvec3& pos, const vsg::dmat4& wtl, QUndoCommand *parent = nullptr)
        : MoveObject(object, pos, parent)
        , _oldWtl(object->worldToLocal)
        , _newWtl(wtl)
    {
        std::string name;
        object->getValue(META_NAME, name);
        setText(QObject::tr("Перемещен объект %1, ECEF %2").arg(name.c_str())
                .arg("X=" + QString::number(pos.x) + " Y=" + QString::number(pos.x) + " Z=" + QString::number(pos.x)));

    }
    void undo() override
    {
        MoveObject::undo();
        _object->worldToLocal = _oldWtl;
    }
    void redo() override
    {
        MoveObject::redo();
        _object->worldToLocal = _newWtl;
    }
    int id() const override
    {
        return QUndoCommand::id();
    }

private:
    const vsg::dmat4 _oldWtl;
    const vsg::dmat4 _newWtl;
};
/*
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
        traj->getValue(META_NAME, name);
        setText(QObject::tr("Удалена траектория %1").arg(name.c_str()));
        it = _topo->trajectories.find(name);
        Q_ASSERT(it != _topo->trajectories.end());
    }
    void undo() override
    {
        std::string name;
        _traj->getValue(META_NAME, name);
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
        transform->getValue(META_NAME, name);
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
