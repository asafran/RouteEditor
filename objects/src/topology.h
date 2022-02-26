#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <vsg/nodes/Group.h>
#include "sceneobjects.h"
#include "signal.h"
#include "trajectory.h"

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

        //void assignBuilder(vsg::ref_ptr<vsg::Builder> builder);

        //std::map<std::string, vsg::ref_ptr<SplineTrajectory>> trajectories;

    protected:

        //Map<std::string, Junction*> junctions;
        //QMap<std::string, Signal*> signal;
        //std::vector<vsg::ref_ptr<Trackside>>     trackside;

    };

    class TopologyVisitor : vsg::ConstVisitor
    {
    public:

        QMap<QString, Junction*> junctions;
        QMap<QString, Signal*> signal;
        QMap<QString, Trajectory*> trajectories;

        void apply(const vsg::Transform& node) override
        {
            if(auto object = node.cast<route::SceneObject>(); object)
            {
                /*
                if(auto traj = node.cast<route::Trajectory>(); traj)
                    trajectories.insert(traj)
                else if(auto conn = node.cast<route::RailConnector>(); conn)
                    connector = conn;
                else if(auto point = node.cast<route::RailPoint>(); point)
                    trackpoint = point;
                 */
            }
        }
    };
}

#endif // TOPOLOGY_H
