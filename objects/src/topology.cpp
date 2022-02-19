#include "topology.h"

namespace route
{
    Topology::Topology() : vsg::Inherit<vsg::Group, Topology>()
    {
    }
    Topology::~Topology()
    {
    }

    void Topology::read(vsg::Input& input)
    {
        Group::read(input);

    }

    void Topology::write(vsg::Output& output) const
    {
        Group::write(output);

    }
/*
    void Topology::assignBuilder(vsg::ref_ptr<vsg::Builder> builder)
    {
        for(const auto &traj : trajectories)
            traj.second->_builder = builder;
    }

    */
}

