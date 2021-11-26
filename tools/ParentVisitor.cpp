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
#include <QDebug>

using namespace vsg;

struct PushPopNode
{
    ParentVisitor::NodePath& nodePath;

    PushPopNode(ParentVisitor::NodePath& np, const Node* node) :
        nodePath(np) { nodePath.push_back(node); }
    ~PushPopNode() { nodePath.pop_back(); }
};

ParentVisitor::ParentVisitor(const vsg::Node* node) :
    child(node)
{

}

void ParentVisitor::apply(const Node& node)
{
    PushPopNode ppn(_nodePath, &node);

    node.traverse(*this);
}

void ParentVisitor::apply(const Group& group)
{
    PushPopNode ppn(_nodePath, &group);
    auto result = std::find(group.children.begin(), group.children.end(), child);
    if( result != group.children.end() )
        pathToChild = _nodePath;
    group.traverse(*this);
}

void ParentVisitor::apply(const LOD& lod)
{
    PushPopNode ppn(_nodePath, &lod);

    for (auto& child : lod.children)
    {
        if (child.node)
        {
            if (this->child == child.node)
                pathToChild = _nodePath;
            child.node->accept(*this);
        }        
    }
}

void ParentVisitor::apply(const PagedLOD& plod)
{
    PushPopNode ppn(_nodePath, &plod);

    for (auto it = plod.children.begin(); it != plod.children.end(); ++it)
    {
        if (it->node)
        {
            if (child == it->node)
                pathToChild = _nodePath;
           it->node->accept(*this);
        }
    }
}

void ParentVisitor::apply(const Switch& sw)
{
    PushPopNode ppn(_nodePath, &sw);

    for (auto it = sw.children.begin(); it != sw.children.end(); ++it)
    {
        if (it->node)
        {
            if (child == it->node)
                pathToChild = _nodePath;
           it->node->accept(*this);
        }
    }
}

void ParentVisitor::apply(const CullNode& cn)
{
    PushPopNode ppn(_nodePath, &cn);
    if( child == cn.child )
        pathToChild = _nodePath;

    cn.traverse(*this);
}
