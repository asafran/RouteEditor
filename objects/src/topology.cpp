#include "topology.h"

namespace route
{
    Topology::Topology() : vsg::Inherit<vsg::Object, Topology>()
    {
    }
    Topology::~Topology()
    {
    }

    void Topology::read(vsg::Input& input)
    {
        Object::read(input);

        //input.readObjects("stations", stations);

        uint32_t numStations = input.readValue<uint32_t>("NumStations");
        stations.clear();
        for (uint32_t i = 0; i < numStations; ++i)
        {
            std::string key;
            vsg::ref_ptr<signalling::Station> station;
            input.read("Key", key);
            input.read("Station", station);
            if (station) stations.insert_or_assign(key, station);
        }
    }

    void Topology::write(vsg::Output& output) const
    {
        Object::write(output);

        //output.writeObjects("stations", stations);

        output.writeValue<uint32_t>("NumStations", stations.size());
        for (const auto& station : stations)
        {
            output.write("Key", station.first);
            output.write("Station", station.second);
        }
    }
/*
    void Topology::assignBuilder(vsg::ref_ptr<vsg::Builder> builder)
    {
        for(const auto &traj : trajectories)
            traj.second->_builder = builder;
    }

    */
}

