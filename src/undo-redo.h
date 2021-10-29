#ifndef CLASS_H
#define CLASS_H

#include "SceneModel.h"

class AddNode : public QUndoCommand
{
public:
    AddNode(vsg::Group *group, vsg::Node *node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _group(group)
        , _node(node)
    {
        std::string name;
        if(!node->getValue(META_NAME, name))
            name = node->className();
        setText(QObject::tr("Новый объект %1").arg(name.c_str()));
    }
    void undo() override
    {
        auto position = std::find(_group->children.cbegin(), _group->children.cend(), _node);
        Q_ASSERT(position != _group->children.end());
        _group->children.erase(position);
    }
    void redo() override
    {
        _group->addChild(_node);
    }
private:
    vsg::ref_ptr<vsg::Group> _group;
    vsg::ref_ptr<vsg::Node> _node;

};

class RemoveNode : public QUndoCommand
{
public:
    RemoveNode(vsg::Group *group, vsg::Node *node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _group(group)
        , _node(node)
    {
        setText(QObject::tr("Удалена нода %1").arg(node->className()));
    }
    void undo() override
    {
        _group->addChild(_node);
    }
    void redo() override
    {
        auto position = std::find(_group->children.cbegin(), _group->children.cend(), _node);
        Q_ASSERT(position != _group->children.end());
        _group->children.erase(position);
    }
private:
    vsg::ref_ptr<vsg::Group> _group;
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
        setText(QObject::tr("Oбъект %1, новое имя ").arg(_oldName.c_str()));
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
    RotateObject(SceneObject *object, vsg::dvec3 vec, double angle, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _object(object)
        , _oldQ(object->quat)
    {
        std::string name;
        int i = 0;
        _object->getValue(META_NAME, name);
        if(_object->getValue(META_TYPE, i))
            setText(QObject::tr("Перемещен объект %1").arg(name.c_str()));
        else
            setText(QObject::tr("Изменена матрица %1").arg(name.c_str()));

        double c = cos(angle * 0.5);
        double s = sin(angle * 0.5);
        vsg::dquat q(s * vec.x, s * vec.y, s * vec.z, c);
        _object->quat = mult(_object->quat, q);
        auto _newMat = vsg::mat4_cast(_object->quat);
        _newMat[3][0] = _object->matrix[3][0];
        _newMat[3][1] = _object->matrix[3][1];
        _newMat[3][2] = _object->matrix[3][2];
    }
    void undo() override
    {
        auto oldMat = vsg::mat4_cast(_oldQ);
        oldMat[3][0] = _newMat[3][0];
        oldMat[3][1] = _newMat[3][1];
        oldMat[3][2] = _newMat[3][2];
        _object->matrix = oldMat;
    }
    void redo() override
    {
        _object->matrix = _newMat;
    }
private:
    vsg::ref_ptr<SceneObject> _object;
    const vsg::dquat _oldQ;
    const vsg::dmat4 _newMat;

};

class MoveObject : public QUndoCommand
{
public:
    MoveObject(vsg::MatrixTransform *transform, const vsg::dmat4& matrix, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _transform(transform)
        , _oldMat(transform->matrix)
        , _newMat(matrix)
    {
        std::string name;
        int i = 0;
        transform->getValue(META_NAME, name);
        if(transform->getValue(META_TYPE, i))
            setText(QObject::tr("Перемещен объект %1").arg(name.c_str()));
        else
            setText(QObject::tr("Изменена матрица %1").arg(name.c_str()));
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
    const vsg::dmat4 _newMat;

};

#endif // CLASS_H
