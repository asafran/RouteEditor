#ifndef PARENTVISITOR_H
#define PARENTVISITOR_H

#include <vsg/nodes/Node.h>
#include "LambdaVisitor.h"

class ParentIndexer : public vsg::Visitor
{
public:
    using NodePath = std::vector<vsg::Node*>;

    void apply(vsg::Node& node) override;

protected:
    NodePath _nodePath = {nullptr};
};

class ParentTracer : public vsg::Visitor
{
public:
    void apply(vsg::Object& object) override;

    std::list<Object*> nodePath;
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
    const vsg::Node *child;
};

template<class T>
int findPosInStruct(const T& parent, const vsg::Node *child)
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
