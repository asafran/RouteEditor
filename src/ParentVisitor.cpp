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

ParentVisitor::ParentVisitor(vsg::Node* node) :
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
    auto vec = group.children;
    auto result = std::find(group.children.begin(), group.children.end(), child);
    if( result != group.children.end() )
        pathToChild = _nodePath;
    group.traverse(*this);
}

void ParentVisitor::apply(const StateGroup& stategroup)
{
    PushPopNode ppn(_nodePath, &stategroup);
    auto result = std::find(stategroup.children.begin(), stategroup.children.end(), child);
    if( result != stategroup.children.end() )
        pathToChild = _nodePath;

    stategroup.traverse(*this);

}

void ParentVisitor::apply(const MatrixTransform& transform)
{
    PushPopNode ppn(_nodePath, &transform);
    auto result = std::find(transform.children.begin(), transform.children.end(), child);
    if( result != transform.children.end() )
        pathToChild = _nodePath;

    transform.traverse(*this);
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

void ParentVisitor::apply(const CullNode& cn)
{
    PushPopNode ppn(_nodePath, &cn);
    if( child == cn.child )
        pathToChild = _nodePath;

    cn.traverse(*this);
}
/*
void ParentVisitor::apply(const VertexIndexDraw& vid)
{
    auto& arrayState = arrayStateStack.back();
    arrayState.apply(vid);
    if (!arrayState.vertices) return;

    if (vid.indices) vid.indices->accept(*this);

    PushPopNode ppn(_nodePath, &vid);

    sphere bound;
    if (!vid.getValue("bound", bound))
    {
        box bb;
        for (auto& vertex : *arrayState.vertices) bb.add(vertex);

        if (bb.valid())
        {
            bound.center = (bb.min + bb.max) * 0.5f;
            bound.radius = length(bb.max - bb.min) * 0.5f;

            // hacky but better to reuse results.  Perhaps use a std::map<> to avoid a breaking const, or make the visitor non const?
            const_cast<VertexIndexDraw&>(vid).setValue("bound", bound);
        }

        // std::cout<<"Computed bounding sphere : "<<bound.center<<", "<<bound.radius<<std::endl;
    }
    else
    {
        // std::cout<<"Found bounding sphere : "<<bound.center<<", "<<bound.radius<<std::endl;
    }

    if (intersects(bound))
    {
        intersectDrawIndexed(vid.firstIndex, vid.indexCount);
    }
}

void ParentVisitor::apply(const Geometry& geometry)
{
    auto& arrayState = arrayStateStack.back();
    arrayState.apply(geometry);
    if (!arrayState.vertices) return;

    if (geometry.indices) geometry.indices->accept(*this);

    PushPopNode ppn(_nodePath, &geometry);

    for (auto& command : geometry.commands)
    {
        command->accept(*this);
    }
}

void ParentVisitor::apply(const BindVertexBuffers& bvb)
{
    arrayStateStack.back().apply(bvb);
}

void ParentVisitor::apply(const BindIndexBuffer& bib)
{
    bib.indices->accept(*this);
}

void ParentVisitor::apply(const vsg::ushortArray& array)
{
    ushort_indices = &array;
    uint_indices = nullptr;
}

void ParentVisitor::apply(const vsg::uintArray& array)
{
    ushort_indices = nullptr;
    uint_indices = &array;
}

void ParentVisitor::apply(const Draw& draw)
{
    PushPopNode ppn(_nodePath, &draw);

    intersectDraw(draw.firstVertex, draw.vertexCount);
}

void ParentVisitor::apply(const DrawIndexed& drawIndexed)
{
    PushPopNode ppn(_nodePath, &drawIndexed);

    intersectDrawIndexed(drawIndexed.firstIndex, drawIndexed.indexCount);
}
*/
