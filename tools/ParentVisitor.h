#ifndef PARENTVISITOR_H
#define PARENTVISITOR_H

#include <vsg/nodes/Node.h>
#include <vsg/traversals/ArrayState.h>
#include "LambdaVisitor.h"

class ParentVisitor : public vsg::ConstVisitor
{
public:
    using NodePath = std::vector<const vsg::Node*>;
    NodePath pathToChild;
    int position;

    explicit ParentVisitor(const vsg::Node* child);

    void apply(const vsg::Node&) override;
    /*virtual void apply(const vsg::Commands&);
    virtual void apply(const vsg::Group&);
    virtual void apply(const vsg::QuadGroup&);
    virtual void apply(const vsg::LOD&);
    virtual void apply(const vsg::PagedLOD&);
    virtual void apply(const vsg::StateGroup&);
    virtual void apply(const vsg::CullGroup&);
    virtual void apply(const vsg::CullNode&);
    virtual void apply(const vsg::MatrixTransform&);
    virtual void apply(const vsg::Transform&);
    virtual void apply(const vsg::Geometry&);
    //virtual void apply(const vsg::VertexIndexDraw&);
    virtual void apply(const vsg::DepthSorted&);
    //virtual void apply(const vsg::Bin&);
    virtual void apply(const vsg::Switch&);*/
protected:

    vsg::ref_ptr<const vsg::Node> child;

    NodePath _nodePath;
};
/*
class ChildVisitor : public vsg::ConstVisitor
{
public:
    ChildVisitor(int in_row)
        : row(in_row) {}

    int row;
    vsg::Node *child;

    using ConstVisitor::apply;

    vsg::Node *operator() (vsg::Node *traversing)
    {
        traversing->accept(*this);
        return child;
    }

    void apply(const vsg::PagedLOD& plod) override
    {
        child = plod.children.at(row).node;
    }
    void apply(const vsg::Switch& sw) override
    {
        child = sw.children.at(row).node;
    }
    void apply(const vsg::Group& group) override
    {
        child = group.children.at(row);
    }
};
*/
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
/*
int findPosInTraj(SceneTrajectory& node, vsg::ref_ptr<const vsg::Node> child)
{
    auto it = std::find(node.traj->getBegin(), node.traj->getEnd(), node);
    Q_ASSERT(it != node.traj->getEnd());
    return std::distance(node.traj->getBegin(), it);
}
*/
#endif // PARENTVISITOR_H
