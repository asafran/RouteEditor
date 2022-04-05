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

        std::vector<std::string> keys;
        std::vector<vsg::ref_ptr<Station>> stations;
        input.read("keys", keys);
        input.read("stations", stations);

        auto key = keys.begin();
        auto station = stations.begin();
        for(; key != keys.end(); ++key, ++station)
            this->stations.insert_or_assign(*key, *station);
    }

    void Topology::write(vsg::Output& output) const
    {
        Group::write(output);

        std::vector<std::string> keys;
        std::vector<vsg::ref_ptr<Station>> stations;

        for(const auto& station : this->stations)
        {
            keys.push_back(station.first);
            stations.push_back(station.second);
        }

        output.write("keys", keys);
        output.write("stations", stations);
    }
/*
    void Topology::assignBuilder(vsg::ref_ptr<vsg::Builder> builder)
    {
        for(const auto &traj : trajectories)
            traj.second->_builder = builder;
    }

    */
}

