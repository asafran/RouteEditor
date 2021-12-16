#include "topology.h"
/*
TrackVisitor::TrackVisitor()
{

}

void TrackVisitor::apply(const vsg::Node& node)
{
    node.traverse(*this);
}

void TrackVisitor::apply(const vsg::Group& node)
{
    if(auto traj = node.cast<SceneTrajectory>(); traj)
    {
        auto group = traj->children.front()->cast<vsg::Group>()->children;
        Sections section;
        for(const auto &child : group)
        {
            //auto track = loader->children.front()->getObject<Track>(META_TRACK); INCREMENT!!!!!
            auto loader = child->cast<RailLoader>();
            //const TrackSection sec;
            section.emplaceBack(loader->children.front()->getObject<Track>(META_TRACK),
                                loader->inclination,
                                traj->transform(vsg::dmat4()) * loader->matrix);
        }
        std::string name;
        traj->getValue(META_NAME, name);
        trajectories.insert(name, Trajectory(section, name, traj->lenght, traj->nextReversed));
    }
}
*/
Topology::Topology() : vsg::Inherit<vsg::Node, Topology>()
  , trajectories()
{

}
Topology::~Topology()
{

}

Trajectories::iterator Topology::insertTraj(vsg::ref_ptr<Trajectory> traj)
{
    std::string name;
    traj->getValue(META_NAME, name);
    return trajectories.insert_or_assign(name, traj).first; //override if contains
}

/*
void Topology::bindTrajs()
{
    for(auto &traj : trajectories)
    {
        traj.second
    }
}
*/
void Topology::read(vsg::Input& input)
{
    Object::read(input);

    std::vector<vsg::ref_ptr<Trajectory>> trajs;
    input.read("trajs", trajs);

    for(auto it = trajs.begin(); it != trajs.end(); ++it )
    {
        std::string name;
        (*it)->getValue(META_NAME, name);
        trajectories.insert_or_assign(name, *it);
    }
}

void Topology::write(vsg::Output& output) const
{
    Object::write(output);

    std::vector<const Trajectory*> trajs;
    for(auto it = trajectories.begin(); it != trajectories.end(); ++it )
        trajs.push_back(it->second.get());

    output.write("trajs", trajs);
}


