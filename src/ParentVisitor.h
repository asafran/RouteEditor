#ifndef PARENTVISITOR_H
#define PARENTVISITOR_H

#include <vsg/nodes/Node.h>
#include <vsg/traversals/ArrayState.h>
#include "vsgGIS/TileDatabase.h"

class ParentVisitor : public vsg::Inherit<vsg::ConstVisitor, ParentVisitor>
    {
    public:
        using NodePath = std::vector<const vsg::Node*>;
        using ArrayStateStack = std::vector<vsg::ArrayState>;
        NodePath pathToChild;

        ParentVisitor(const vsg::Node* child);

        //
        // handle traverse of the scene graph
        //
        void apply(const vsg::Node& node) override;
        void apply(const vsg::Group& group) override;
        void apply(const vsg::StateGroup& stategroup) override;
        void apply(const vsg::MatrixTransform& transform) override;
        void apply(const vsg::LOD& lod) override;
        void apply(const vsg::PagedLOD& plod) override;
        void apply(const vsg::CullNode& cn) override;

    protected:

        vsg::ref_ptr<const vsg::Node> child;

        NodePath _nodePath;
    };

#endif // PARENTVISITOR_H
