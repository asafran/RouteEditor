#ifndef UNDO_H
#define UNDO_H

#include "SceneObjectsModel.h"
#include "trajectory.h"
#include "signals.h"
#include <unordered_set>

class AddSceneObject : public QUndoCommand
{
public:
    AddSceneObject(SceneModel *model,
            const QModelIndex &group,
            vsg::ref_ptr<route::MVCObject> node,
            QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _model(model)
        , _group(group)
        , _node(node)
    {
        auto name = node->getName();
        if(name.isEmpty())
            name = node->className();
        setText(QObject::tr("Новый объект %1").arg(name));
    }
    AddSceneObject(SceneModel *model,
            route::SceneGroup *group,
            vsg::ref_ptr<route::MVCObject> node,
            QUndoCommand *parent = nullptr)
        : AddSceneObject(model, model->index(group), node, parent)
    {
    }
    void undo() override
    {
        _model->removeNode(_model->index(_row, 0, _group));
    }
    void redo() override
    {
        _row = _model->addNode(_group, _node);
    }
private:
    SceneModel *_model;
    int _row;
    const QModelIndex _group;
    vsg::ref_ptr<route::MVCObject> _node;

};

class AddSignal : public QUndoCommand
{
public:
    AddSignal(route::Connector *rc,
              vsg::ref_ptr<signalling::Signal> sig,
              QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _rc(rc)
        , _sig(sig) {}

    void undo() override
    {
        _rc->setSignal({});
    }
    void redo() override
    {
        _rc->setSignal(_sig);
    }
protected:
    vsg::ref_ptr<route::Connector> _rc;
    vsg::ref_ptr<signalling::Signal> _sig;
};

class RemoveSignal : public AddSignal
{
public:
    RemoveSignal(route::Connector *rc, QUndoCommand *parent = nullptr)
        : AddSignal(rc, rc->signal(), parent)
    {
        setText(QObject::tr("Удален сигнал, направление назад"));
    }

    void undo() override
    {
        AddSignal::redo();
    }
    void redo() override
    {
        AddSignal::undo();
    }
};

class RemoveNode : public QUndoCommand
{
public:
    RemoveNode(SceneModel *model, const QModelIndex &index, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _model(model)
        , _node(static_cast<route::MVCObject*>(index.internalPointer()))
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
    }
    void redo() override
    {
        _model->removeNode(_model->index(_row, 0, _group));
    }
private:
    SceneModel *_model;
    int _row;
    vsg::ref_ptr<route::MVCObject> _node;
    const QModelIndex _group;

};

class RenameObject : public QUndoCommand
{
public:
    RenameObject(route::MVCObject *object, const QString &name, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _newName(name)
    {
        _oldName = object->getName();
        setText(QObject::tr("Oбъект %1, новое имя %2").arg(_oldName).arg(name));
    }
    RenameObject(const QModelIndex &index, const QString &name, QUndoCommand *parent = nullptr)
        : RenameObject(static_cast<route::MVCObject*>(index.internalPointer()), name, parent)
    {
    }
    void undo() override
    {
        _object->setName(_oldName);
    }
    void redo() override
    {
        _object->setName(_newName);
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
    vsg::ref_ptr<route::MVCObject> _object;
    QString _oldName;
    QString _newName;

};
/*
class TransformObject : public QUndoCommand
{
public:
    TransformObject(route::MVCObject *object, const vsg::dmat4 &transform, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _oldMat(object->getTransform())
        , _newMat(transform)
    {
        auto name = object->getName();
        setText(QObject::tr("Повернут/перемещен объект %1").arg(name));
    }
    void undo() override
    {
        _object->setTransform(_oldMat);
    }
    void redo() override
    {
        _object->setTransform(_newMat);
    }
    int id() const override
    {
        return 2;
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const TransformObject*>(other);
        if(rcmd->_object != _object)
            return false;
        _newMat = rcmd->_newMat;
        return true;
    }
private:
    vsg::ref_ptr<route::MVCObject> _object;
    const vsg::dmat4 _oldMat;
    vsg::dmat4 _newMat;
};

class TransformObjects : public QUndoCommand
{
public:
    TransformObjects(QSet<QModelIndex> selectedObjects, const vsg::dmat4 &delta, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _selectedObjects(selectedObjects)
        , _delta(delta)
        , _inverse(vsg::inverse(delta))
    {
    }

    void undo() override
    {
        for (auto& index : qAsConst(_selectedObjects)) {
            auto object = static_cast<route::MVCObject*>(index.internalPointer());
            auto transform = object->getTransform() * _inverse;
            object->setTransform(transform);
        }
    }
    void redo() override
    {
        for (auto& index : qAsConst(_selectedObjects)) {
            auto object = static_cast<route::MVCObject*>(index.internalPointer());
            auto transform = object->getTransform() * _delta;
            object->setTransform(transform);
        }
    }
    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const TransformObjects*>(other);
        if(rcmd->_selectedObjects != _selectedObjects)
            return false;
        _delta = rcmd->_delta * _delta;
        _inverse = vsg::inverse(_delta);
        return true;
    }
protected:
    QSet<QModelIndex> _selectedObjects;
    vsg::dmat4 _delta;
    vsg::dmat4 _inverse;
};
*/

class RotateObject : public QUndoCommand
{
public:
    RotateObject(route::MVCObject* object, const vsg::dquat &to, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _final(to)
        , _initial(object->getRotation())
    {
        auto name = object->getName();
        setText(QObject::tr("Повернут объект %1").arg(name));
    }

    void undo() override
    {
        _object->setRotation(_initial);
    }
    void redo() override
    {
        _object->setRotation(_final);
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
        _final = rcmd->_final;
        _initial = rcmd->_initial;
        return true;
    }
protected:

    vsg::ref_ptr<route::MVCObject> _object;
    vsg::dquat _final;
    vsg::dquat _initial;
};

class MoveObject : public QUndoCommand
{
public:
    MoveObject(route::MVCObject* object, const vsg::dvec3 &to, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _final(to)
        , _initial(object->getPosition())
    {
        auto name = object->getName();
        setText(QObject::tr("Перемещен объект %1").arg(name));
    }

    void undo() override
    {
        _object->setPosition(_initial);
    }
    void redo() override
    {
        _object->setPosition(_final);
    }

    int id() const override
    {
        return 3;
    }

    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const MoveObject*>(other);
        if(rcmd->_object != _object)
            return false;
        _final = rcmd->_final;
        //_initial = rcmd->_initial;
        return true;
    }
protected:

    vsg::ref_ptr<route::MVCObject> _object;
    vsg::dvec3 _final;
    vsg::dvec3 _initial;
};

class RotateObjects : public QUndoCommand
{
public:
    RotateObjects(QSet<QModelIndex> selectedObjects, const vsg::dquat &to, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _selectedObjects(selectedObjects)
        , _final(to)
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        _initial = object->getRotation();

        setText(QObject::tr("Повернуты %1 объектов").arg(_selectedObjects.size()));
    }

    void undo() override
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        auto delta = _initial * tools::inverse(object->getRotation());
        applyRotation(delta);
    }
    void redo() override
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        auto delta =  _final * tools::inverse(object->getRotation());
        applyRotation(delta);
    }

    int id() const override
    {
        return 3;
    }

    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const RotateObjects*>(other);
        if(rcmd->_selectedObjects != _selectedObjects)
            return false;
        _final = rcmd->_final;
        _initial = rcmd->_initial;
        return true;
    }
protected:

    void applyRotation(const vsg::dquat &delta)
    {
        for (auto& index : qAsConst(_selectedObjects)) {
            auto object = static_cast<route::MVCObject*>(index.internalPointer());
            auto rot = delta * object->getRotation();
            object->setRotation(rot);
        }
    }

    QSet<QModelIndex> _selectedObjects;
    vsg::dquat _final;
    vsg::dquat _initial;
};

class MoveObjects : public QUndoCommand
{
public:
    MoveObjects(QSet<QModelIndex> selectedObjects, const vsg::dvec3 &delta, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _selectedObjects(selectedObjects)
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        _initial = object->getPosition();
        _final = _initial + delta;

        setText(QObject::tr("Перемещены %1 объектов").arg(_selectedObjects.size()));
    }

    void undo() override
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        auto delta = _initial - object->getPosition();
        applyPosition(delta);
    }
    void redo() override
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        auto delta = _final - object->getPosition();
        applyPosition(delta);
    }

    int id() const override
    {
        return 4;
    }

    bool mergeWith(const QUndoCommand *other) override
    {
        if (other->id() != id())
            return false;
        auto rcmd = static_cast<const MoveObjects*>(other);
        if(rcmd->_selectedObjects != _selectedObjects)
            return false;
        _final = rcmd->_final;
        _initial = rcmd->_initial;
        return true;
    }
protected:

    void applyPosition(const vsg::dvec3 &delta)
    {
        for (auto& index : qAsConst(_selectedObjects)) {
            auto object = static_cast<route::MVCObject*>(index.internalPointer());
            auto pos = object->getPosition() + delta;
            object->setPosition(pos);
        }
    }

    QSet<QModelIndex> _selectedObjects;
    vsg::dvec3 _final;
    vsg::dvec3 _initial;
};

/*
class MoveObjectOnTraj : public QUndoCommand
{
public:
    MoveObjectOnTraj(route::MVCObject *object, double coord, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _newPos(coord)
        , _parent(traj)
    {
        auto name = object->getName();
        setText(QObject::tr("Перемещен объект %1, путевая коодината %2").arg(name).arg(QString::number(coord)));

        _oldPos = traj->childrenCoords.at(object);
    }
    void undo() override
    {
        _parent->childrenCoords.at(_object) = _oldPos;
    }
    void redo() override
    {
        _parent->childrenCoords.at(_object) = _newPos;
    }
    int id() const override
    {
        return 6;
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
    vsg::ref_ptr<route::Trajectory> _parent;
    vsg::ref_ptr<route::MVCObject> _object;
    double _oldPos;
    double _newPos;
};
*/

class ConnectRails : public QUndoCommand
{
public:
    ConnectRails(route::Connector *conn1, route::Connector *conn2, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , _conn1(conn1)
        , _conn2(conn2)
    {
        setText(QObject::tr("Соединены траектории %1 и %2").arg(conn1->trajectory->getName()).arg(conn2->trajectory->getName()));
    }
    void undo() override
    {
        _conn1->disconnect();
    }
    void redo() override
    {
        _conn1->connect(_conn2);
    }
protected:
    vsg::ref_ptr<route::Connector> _conn1;
    vsg::ref_ptr<route::Connector> _conn2;
};

class DisconnectRails : public ConnectRails
{
public:
    DisconnectRails(route::Connector *conn, QUndoCommand *parent = nullptr)
        : ConnectRails(conn, conn->connected(), parent)
    {
        setText(QObject::tr("Разъеденины траектории %1 и %2").arg(_conn1->trajectory->getName()).arg(_conn2->trajectory->getName()));
    }
    void undo() override
    {
        ConnectRails::redo();
    }
    void redo() override
    {
        ConnectRails::undo();
    }
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

#endif // UNDO_H
