#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <vsg/nodes/Group.h>
#include "sceneobjects.h"
#include "trajectory.h"
/*
class TrackVisitor : public vsg::Inherit<vsg::ConstVisitor, TrackVisitor>
{
public:
    explicit TrackVisitor();

    void apply(const vsg::Node& node) override;
    void apply(const vsg::Group& group) override;

    QMap<std::string, Trajectory> trajectories;
};

class Topology;

class TopologySingleton
{
private:
    TopologySingleton() {}
    TopologySingleton(const TopologySingleton&);
    TopologySingleton& operator=(const TopologySingleton&);

public:
    vsg::ref_ptr<Topology> _ptr;
    static TopologySingleton& instance(){ static TopologySingleton g_Instance; return g_Instance; }
    static Topology* topology(){ static TopologySingleton g_Instance; return g_Instance._ptr; }
};
*/
using Trajectories = std::map<std::string, vsg::ref_ptr<Trajectory>>;

class Topology : public vsg::Inherit<vsg::Node, Topology>
{
public:
    Topology();

    virtual ~Topology();

    Trajectories::iterator insertTraj(vsg::ref_ptr<Trajectory> traj);

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    template<class N, class V>
    static void t_traverse(N& node, V& visitor)
    {
        for (auto it = node.trajectories.begin(); it != node.trajectories.end(); ++it)
            it->second->accept(visitor);
    }

    void traverse(vsg::Visitor& visitor) override { t_traverse(*this, visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { t_traverse(*this, visitor); }

    //void bindTrajs();

    Trajectories trajectories;
    //std::map<std::string, vsg::ref_ptr<Junction>> junctions;
    //std::map<std::string, AnimatedTrackside> trackside;
    //std::map<std::string, Signal> signal;

protected:

    //std::vector<vsg::ref_ptr<Trackside>>     trackside;

};

#endif // TOPOLOGY_H
