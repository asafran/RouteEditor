#include "signal.h"

namespace route
{
    Signal::Signal(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box)
        : vsg::Inherit<SceneObject, Signal>(loaded, box)
    {
    }
}
