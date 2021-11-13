#ifndef UNDO_H
#define UNDO_H

#include "SceneModel.h"

class AddNode : public QUndoCommand
{
public:
    AddNode(SceneModel *model, const QModelIndex &group, vsg::ref_ptr<vsg::Node> node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _model(model)
        , _group(group)
        , _node(node)
    {
        std::string name;
        node->getValue(META_NAME, name);
        if(name.empty())
            name = node->className();
        setText(QObject::tr("Новый объект %1").arg(name.c_str()).arg(reinterpret_cast<quint64>(node.get()), 0, 16));
    }
    void undo() override
    {
        _model->removeNode(_group, _node);
    }
    void redo() override
    {
        _model->addNode(_group, _node);
    }
private:
    SceneModel *_model;
    const QModelIndex _group;
    vsg::ref_ptr<vsg::Node> _node;

};

class RemoveNode : public QUndoCommand
{
public:
    RemoveNode(SceneModel *model, const QModelIndex &group, vsg::Node *node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _model(model)
        , _group(group)
        , _node(node)
    {
        std::string name;
        node->getValue(META_NAME, name);
        if(name.empty())
            name = node->className();
        setText(QObject::tr("Удален объект %1").arg(name.c_str()).arg(reinterpret_cast<quint64>(node), 0, 16));
    }
    void undo() override
    {
        _model->addNode(_group, _node);
    }
    void redo() override
    {
        _model->removeNode(_group, _node);
    }
private:
    SceneModel *_model;
    const QModelIndex _group;
    vsg::ref_ptr<vsg::Node> _node;

};
/*
class AddObject : public QUndoCommand
{
public:
    AddObject(vsg::Group *group, vsg::SceneObject *object, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _group(group)
        , _object(object)
    {
        std::string name;
        object->getValue(META_NAME, name);
        setText(QObject::tr("Новый объект %1").arg(name.c_str()));
    }
    void undo() override
    {
        auto position = std::find(_group->children.cbegin(), _group->children.cend(), _object);
        Q_ASSERT(position != _group->children.end());
        _group->children.erase(position);
    }
    void redo() override
    {
        _group->addChild(_object);
    }
private:
    vsg::ref_ptr<vsg::Group> _group;
    vsg::ref_ptr<vsg::SceneObject> _object;

};
*/
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
class RotateObject : public QUndoCommand
{
public:
    RotateObject(SceneObject *object, vsg::dquat q, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _oldQ(object->quat)
        , _newQ(mult(_object->quat, q))
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
private:
    vsg::ref_ptr<SceneObject> _object;
    const vsg::dquat _oldQ;
    const vsg::dquat _newQ;
};

class MoveObject : public QUndoCommand
{
public:
    MoveObject(vsg::MatrixTransform *transform, const vsg::dvec3& pos, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _transform(transform)
        , _oldMat(transform->matrix)
    {
        std::string name;
        transform->getValue(META_NAME, name);
        setText(QObject::tr("Перемещен объект %1, ECEF %2").arg(name.c_str())
                .arg("X=" + QString::number(pos.x) + " Y=" + QString::number(pos.x) + " Z=" + QString::number(pos.x)));

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
    vsg::ref_ptr<vsg::MatrixTransform> _transform;
    const vsg::dmat4 _oldMat;
    vsg::dmat4 _newMat;

};
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
