#ifndef CLASS_H
#define CLASS_H

#include <QUndoStack>
#include <vsg/nodes/Group.h>

class AddNode : public QUndoCommand
{
public:
    AddNode(vsg::ref_ptr<vsg::Group> group, vsg::ref_ptr<vsg::Node> node)
        : _group(group)
        , _node(node)
    {
        setText(QObject::tr("Новый объект %1").arg(node->className()));
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

#endif // CLASS_H
