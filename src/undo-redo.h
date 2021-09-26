#ifndef CLASS_H
#define CLASS_H

#include <QUndoStack>
#include <vsg/nodes/MatrixTransform.h>
#include "metainf.h"

class AddNode : public QUndoCommand
{
public:
    AddNode(vsg::ref_ptr<vsg::Group> group, const vsg::ref_ptr<vsg::Node> node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _group(group)
        , _node(node)
    {
        setText(QObject::tr("Новая нода %1").arg(node->className()));
        group->addChild(vsg::ref_ptr<vsg::Node>(node));
    }
    void undo() override
    {
        auto position = std::find(_group->children.cbegin(), _group->children.cend(), _node);
        Q_ASSERT(position != _group->children.end());
        _group->children.erase(position);
    }
    void redo() override
    {
        _group->addChild(vsg::ref_ptr<vsg::Node>(_node));
    }
private:
    vsg::ref_ptr<vsg::Group> _group;
    vsg::ref_ptr<vsg::Node> _node;

};

class AddObject : public QUndoCommand
{
public:
    AddObject(vsg::Group *group, vsg::Node *node, QUndoCommand *parent = nullptr) : QUndoCommand(parent)
        , _group(group)
        , _transform(vsg::MatrixTransform::create())
    {
        std::string name;
        node->getValue(META_NAME, name);
        setText(QObject::tr("Новый объект %1").arg(name.c_str()));
        _transform->addChild(vsg::ref_ptr<vsg::Node>(node));
    }
    void undo() override
    {
        auto position = std::find(_group->children.cbegin(), _group->children.cend(), _transform);
        Q_ASSERT(position != _group->children.end());
        _group->children.erase(position);
    }
    void redo() override
    {
        _group->addChild(_transform);
    }
private:
    vsg::ref_ptr<vsg::Group> _group;
    vsg::ref_ptr<vsg::MatrixTransform> _transform;

};

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

#endif // CLASS_H
