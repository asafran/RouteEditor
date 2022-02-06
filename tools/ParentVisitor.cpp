#include "ParentVisitor.h"

#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/Draw.h>
#include <vsg/commands/DrawIndexed.h>
#include <vsg/maths/transform.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/Geometry.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/state/GraphicsPipeline.h>
#include "sceneobjects.h"

#include <execution>


struct PushPopNode
{
    ParentVisitor::NodePath& nodePath;

    PushPopNode(ParentVisitor::NodePath& np, const vsg::Node* node) :
        nodePath(np) { nodePath.push_back(node); }
    ~PushPopNode() { nodePath.pop_back(); }
};

ParentVisitor::ParentVisitor(const ParentVisitor &p)
    : operation(p.operation)
    , _child(p._child)
    , _stack(p._stack)
{
    traversalMask = p.traversalMask;
}

ParentVisitor::ParentVisitor(vsg::observer_ptr<FindParent> op, const vsg::Node *child)
    : operation(op)
    , _child(child)
{
}

void ParentVisitor::apply(const vsg::Node& node)
{
    PushPopNode ppn(_stack, &node);
    if(&node == _child)
    {
        vsg::ref_ptr<FindParent> o(operation);
        o->found(_stack);
    }
    else
        node.traverse(*this);
}
void ParentVisitor::apply(const vsg::Group& node)
{
    PushPopNode ppn(_stack, &node);
    if(&node == _child)
    {
        vsg::ref_ptr<FindParent> o(operation);
        o->found(_stack);
    }
    else if(node.children.size() == 1)
    {
        node.traverse(*this);
    }
    else
    {
        {
            vsg::ref_ptr<FindParent> o(operation);
            std::for_each(std::next(node.children.begin()), node.children.end(), [o, this](auto child)
            {
                o->traverseThreads->add(TraverseOperation::create(ParentVisitor(*this), child.get()));
            });
        }
        node.children.front()->accept(*this);
    }
}

FindParent::FindParent(vsg::ref_ptr<vsg::ActivityStatus> status)
    : _status(status)
{
    traverseThreads = vsg::OperationThreads::create(16, status);
}

void FindParent::found(const ParentVisitor::NodePath &_stack)
{
    pathToChild = _stack;
    _status->set(false);
}

void FindParent::apply(const vsg::Node *node, const vsg::Node *child, uint32_t mask)
{
    ParentVisitor pv(vsg::observer_ptr<FindParent>(this), child);
    pv.traversalMask = mask;
    node->accept(pv);

    while(_status->active());

    traverseThreads->stop();
}
/*
ParentVisitor::TraverseOperation::TraverseOperation(const TraverseOperation &to, const vsg::Node *in_node)
    : child(to.child)
    , node(in_node)
    , pv(to.pv)
    , nodePath(to.nodePath)
{
}
*/
TraverseOperation::TraverseOperation(const ParentVisitor &in_pv, const vsg::Node *in_node)
    : node(in_node)
    , pv(in_pv)

{
}

void TraverseOperation::run()
{
    node->accept(pv);
}
/*
void ParentVisitor::TraverseOperation::apply(const vsg::Node& node)
{
    PushPopNode ppn(nodePath, &node);
    if(&node == child)
    {
        vsg::ref_ptr<ParentVisitor> pvisitor(pv);
        pvisitor->found(nodePath);
    }
    else
        node.traverse(*this);
}

void ParentVisitor::TraverseOperation::apply(const vsg::Group& node)
{
    PushPopNode ppn(nodePath, &node);
    if(&node == child)
    {
        vsg::ref_ptr<ParentVisitor> pvisitor(pv);
        pvisitor->found(nodePath);
    }
    else if(node.children.size() == 1)
    {
        node.traverse(*this);
    }
    else
    {
        vsg::ref_ptr<ParentVisitor> pvisitor(pv);
        std::for_each(std::next(node.children.begin()), node.children.end(), [pvisitor, this](auto child)
        {
            pvisitor->traverseThreads->add(TraverseOperation::create(*this, child.get()));
        });
        node.children.front()->accept(*this);
    }
}
*/
FindPositionVisitor::FindPositionVisitor(const vsg::Node* node) : vsg::ConstVisitor()//ConstSceneObjectsVisitor()
    , child(node)
{

}
/*
void FindPositionVisitor::apply(const SectionTrajectory &traj)
{
    auto it = std::find(traj.traj->getBegin(), traj.traj->getEnd(), child);
    Q_ASSERT(it != traj.traj->getEnd());
    position = std::distance(traj.traj->getBegin(), it);
}
*/
void FindPositionVisitor::apply(const vsg::Group& group)
{
    auto it = std::find(group.children.cbegin(), group.children.cend(), child);
    if(it != group.children.end())
        return;
    position = std::distance(group.children.cbegin(), it);
}

void FindPositionVisitor::apply(const vsg::LOD& lod)
{
    position = findPosInStruct(lod, child);
}

void FindPositionVisitor::apply(const vsg::PagedLOD& plod)
{
    position = findPosInStruct(plod, child);
}

void FindPositionVisitor::apply(const vsg::Switch& sw)
{
    auto it = sw.children.begin();
    for ( ;it != sw.children.end(); ++it)
    {
        if ((traversalMask & (overrideMask | it->mask)) != 0)
        {
            if (it->node == child)
                break;
            ++position;
        }
        else
            continue;
    }
}

void FindPositionVisitor::apply(const vsg::CullNode& cn)
{
    position = 0;
}


