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
#include "Constants.h"

#include <execution>


struct PushPopNode
{
    ParentIndexer::NodePath& nodePath;

    PushPopNode(ParentIndexer::NodePath& np, vsg::Node* node) :
        nodePath(np) { nodePath.push_back(node); }
    ~PushPopNode() { nodePath.pop_back(); }
};

void ParentIndexer::apply(vsg::Node& node)
{
    node.setValue(app::PARENT, _nodePath.back());
    PushPopNode ppn(_nodePath, &node);
    node.traverse(*this);
}

void ParentTracer::apply(vsg::Object &object)
{
    vsg::Node *parent = nullptr;
    if(object.getValue(app::PARENT, parent) && parent)
    {
        nodePath.push_front(parent);
        parent->accept(*this);
    }
}

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
    if(it == group.children.end())
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


