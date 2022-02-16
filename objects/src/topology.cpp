#include "topology.h"

namespace route
{
    Topology::Topology() : vsg::Inherit<vsg::Object, Topology>()
      , trajectories()
    {

    }
    Topology::~Topology()
    {

    }

    std::map<std::string, vsg::ref_ptr<SplineTrajectory>>::iterator Topology::insertTraj(vsg::ref_ptr<SplineTrajectory> traj)
    {
        std::string name;
        traj->getValue(app::NAME, name);
        return trajectories.insert_or_assign(name, traj).first; //override if contains
    }

    void Topology::read(vsg::Input& input)
    {
        Object::read(input);

        std::vector<vsg::ref_ptr<SplineTrajectory>> trajs;
        input.read("trajs", trajs);

        for(const auto &traj : trajs)
        {
            std::string name;
            traj->getValue(app::NAME, name);
            trajectories.insert_or_assign(name, traj);
        }
    }

    void Topology::write(vsg::Output& output) const
    {
        Object::write(output);

        std::vector<const Trajectory*> trajs;
        for(const auto &traj : trajectories)
            trajs.push_back(traj.second.get());

        output.write("trajs", trajs);
    }

    void Topology::assignBuilder(vsg::ref_ptr<vsg::Builder> builder)
    {
        for(const auto &traj : trajectories)
            traj.second->_builder = builder;
    }
}

