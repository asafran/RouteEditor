#ifndef PARENTVISITOR_H
#define PARENTVISITOR_H

#include <vsg/nodes/Node.h>
#include <vsg/traversals/ArrayState.h>
#include <vsg/threading/ActivityStatus.h>
#include <vsg/threading/OperationThreads.h>
#include <vsg/threading/OperationQueue.h>
#include "LambdaVisitor.h"

class FindParent;

class ParentVisitor : public vsg::ConstVisitor
{
public:
    using NodePath = std::vector<const vsg::Node*>;

    ParentVisitor(vsg::observer_ptr<FindParent> op, const vsg::Node *child);

    ParentVisitor(const ParentVisitor &p);

    void apply(const vsg::Group &) override;
    void apply(const vsg::Node &) override;

    vsg::observer_ptr<FindParent> operation;

protected:
    const vsg::Node *_child;

    NodePath _stack;
};

class FindParent : public vsg::Inherit<vsg::Object, FindParent>
{
public:
    explicit FindParent(vsg::ref_ptr<vsg::ActivityStatus> status = vsg::ActivityStatus::create());

    ParentVisitor::NodePath pathToChild;

    vsg::ref_ptr<vsg::OperationThreads> traverseThreads;

    void found(const ParentVisitor::NodePath &_stack);

    void apply(const vsg::Node* node, const vsg::Node *child, uint32_t mask);

protected:

    vsg::ref_ptr<vsg::ActivityStatus> _status;
};

struct TraverseOperation : public vsg::Inherit<vsg::Operation, TraverseOperation>
{
    TraverseOperation(const ParentVisitor &in_pv, const vsg::Node *in_node);

    const vsg::Node *node;
    ParentVisitor pv;

    void run() override;
};


class FindPositionVisitor : public vsg::ConstVisitor//ConstSceneObjectsVisitor
{
public:
    int position = 0;

    explicit FindPositionVisitor(const vsg::Node* child);


    //void apply(const vsg::Node& node) override;
    //void apply(const SectionTrajectory& traj) override;
    void apply(const vsg::Switch& sw) override;
    void apply(const vsg::Group& group) override;
    void apply(const vsg::LOD& lod) override;
    void apply(const vsg::PagedLOD& plod) override;
    void apply(const vsg::CullNode& cn) override;

    int operator()(const vsg::Node* node) { node->accept(*this); return position; }

protected:
    vsg::ref_ptr<const vsg::Node> child;
};

template<class T>
int findPosInStruct(const T& parent, vsg::ref_ptr<const vsg::Node> child)
{
    auto it = parent.children.begin();
    for ( ;it != parent.children.end(); ++it)
    {
        if (it->node == child)
            break;
    }
    //Q_ASSERT(it != parent.children.end());
    return std::distance(parent.children.begin(), it);
}
#endif // PARENTVISITOR_H
