#ifndef SIGNAL_H
#define SIGNAL_H

#include "sceneobjects.h"
#include <vsg/nodes/Switch.h>
#include "Constants.h"

namespace route
{
    class Signal : public vsg::Inherit<SceneObject, Signal>
    {
    public:
        Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box);
        Signal();

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor);  }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { SceneObject::traverse(visitor); t_traverse(*this, visitor); }

        enum Mask
        {

        };

    protected:

        vsg::ref_ptr<vsg::Switch> _lights;

    };
}
#endif // SIGNAL_H
