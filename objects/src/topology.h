#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <vsg/nodes/Group.h>
#include "sceneobjects.h"
#include "signal.h"
#include "trajectory.h"
#include "interlocking.h"

namespace route
{
    class Topology : public vsg::Inherit<vsg::Group, Topology>
    {
    public:
        Topology();

        virtual ~Topology();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void traverse(vsg::Visitor& visitor) override
        { if ((visitor.traversalMask & (visitor.overrideMask | route::Tracks)) != vsg::MASK_OFF) Group::traverse(visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override
        { if ((visitor.traversalMask & (visitor.overrideMask | route::Tracks)) != vsg::MASK_OFF) Group::traverse(visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { Group::traverse(visitor); }

        std::map<std::string, vsg::ref_ptr<signalling::Station>> stations;

    };

    class TopologyVisitor : public vsg::Visitor
    {
    public:
        vsg::ref_ptr<vsg::Builder> builder;

        void apply(vsg::Group& node) override
        {
            if(auto traj = node.cast<route::SplineTrajectory>(); traj)
                traj->_builder = builder;
        }
    };

    template<typename T>
    void write(const char* propertyName, const std::vector<T>& values)
    {
        uint32_t numElements = static_cast<uint32_t>(values.size());
        write(propertyName, numElements);

        for (uint32_t i = 0; i < numElements; ++i)
        {
            write("element", values[i]);
        }
    }
}

#endif // TOPOLOGY_H
