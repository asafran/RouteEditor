#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <vsg/io/read.h>
#include    "topology.h"

TrackSection::TrackSection(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, vsg::dmat4 in_mat, Trajectory *parent)
    : vsg::Inherit<vsg::Transform, TrackSection>()
    , matrix(in_mat)
    , filename(in_file)
    , traj(parent)
{
    track = loaded->getObject<Track>(META_TRACK);
    addChild(loaded);
}

void TrackSection::read(vsg::Input& input)
{
    Node::read(input);

    input.read("filename", filename);
    auto node = vsg::read_cast<vsg::Node>(filename, input.options);
    addChild(node);
    track = node->getObject<Track>(META_TRACK);

    input.read("incl", inclination);
    input.read("matrix", matrix);
    input.read("filename", filename);
    //input.read("prev", prev);
    input.read("parent", traj);

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}

void TrackSection::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("filename", filename);

    output.write("incl", inclination);
    output.write("matrix", matrix);
    output.write("filename", filename);
    //output.write("prev", prev);
    output.write("parent", traj);

    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}
vsg::dmat4 TrackSection::transform(const vsg::dmat4 &m) const
{
    /*
    auto pincl = vsg::rotate(atan(prev->inclination / 1000), vsg::dvec3(1.0, 0.0, 0.0));
    auto incl = vsg::rotate(atan(inclination / 1000), vsg::dvec3(1.0, 0.0, 0.0));

    return m * prev->world(prev->track->lenght) * vsg::inverse(pincl) * incl;
    */
    return m * matrix;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

SectionTrajectory::SectionTrajectory(std::string name, const vsg::dmat4 &offset)
  : SectionTrajectory()
{
    matrixStack.front() = offset;
}

SectionTrajectory::SectionTrajectory()
  : vsg::Inherit<Trajectory, SectionTrajectory>()
  , lenght(0.0)
  , frontReversed(false)
  , sections()
{
    matrixStack.push_back(vsg::dmat4());
}
SectionTrajectory::~SectionTrajectory()
{
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void SectionTrajectory::read(vsg::Input& input)
{
    Object::read(input);

    input.read("sections", sections);
    input.read("lenght", lenght);
    input.read("frontReversed", frontReversed);
    input.read("matrixStack", matrixStack);

    input.read("fwd", fwdTraj);
}

void SectionTrajectory::write(vsg::Output& output) const
{
    Object::write(output);

    output.write("sections", sections);
    output.write("lenght", lenght);
    output.write("frontReversed", frontReversed);
    output.write("matrixStack", matrixStack);

    output.write("fwd", fwdTraj);
}

void SectionTrajectory::addTrack(vsg::ref_ptr<vsg::Node> node, const std::string &name)
{
    auto loader = TrackSection::create(node, name, matrixStack.back(), this);

    matrixStack.emplace_back(matrixStack.back() * loader->track->transform(loader->track->lenght));
    lenght += loader->track->lenght;

    //track->ltw = vsg::inverse(vsg::translate(position)) * vsg::inverse(next);
    sections.push_back(loader);
}

void SectionTrajectory::removeTrack(int section)
{

    sections.pop_back();
}

void SectionTrajectory::recalculatePositions()
{
    auto matrix = matrixStack.begin();
    auto it = sections.begin();

    vsg::dmat4 next;
    for(; it != sections.end(); ++it)
    {
        auto inclination = vsg::rotate(atan((*it)->inclination / 1000), vsg::dvec3(1.0, 0.0, 0.0));

        (*it)->matrix = *matrix * next * inclination;
        matrix++;
        next = next * inclination * (*matrix * vsg::inverse(*(matrix - 1))) * vsg::inverse(inclination) * vsg::inverse(*matrix * vsg::inverse(*(matrix - 1)));
        //next[3] = 1.0;

     }
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
vsg::dmat4 SectionTrajectory::getPosition(double x) const
{
    auto section = getSection(x);

    return (*section.first)->world(section.second);
}

std::pair<Sections::const_iterator, double> SectionTrajectory::getSection(double x) const
{
    if(x > lenght)
        x = lenght;

    double tracks_coord = 0.0;
    auto it = sections.begin();

    while ((tracks_coord + (*it)->track->lenght) < x)
    {
        tracks_coord += (*it)->track->lenght;
        ++it;
    }

    return std::make_pair(it, x - tracks_coord);
}

SceneTrajectory::SceneTrajectory()
    : vsg::Inherit<vsg::Node, SceneTrajectory>()
{
}
SceneTrajectory::SceneTrajectory(Trajectory *trajectory)
    : SceneTrajectory()
{
    traj = trajectory;
}

SceneTrajectory::~SceneTrajectory() {}

void SceneTrajectory::read(vsg::Input& input)
{
    Node::read(input);

    std::string name;
    input.read("trajName", name);

    traj = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);

    /*
    //input.read("files", files);

    vsg::dmat4 matrix;
    for(auto &file : files)
    {
        vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
        vsg::Path filename = vsg::findFile(file, searchPaths);
        auto tracknode = vsg::read_cast<vsg::Node>(filename);
        auto track = tracknode->getObject<Track>("Trk");
        tracks.emplace_back(track);
        auto transform = vsg::MatrixTransform::create(matrix);
        transform->addChild(tracknode);
        addChild(transform);
        track->ltw = vsg::inverse(vsg::translate(position)) * vsg::inverse(matrix);
        matrix = vsg::translate(track->position(track->lenght)) * matrix;
    }
    */
}

void SceneTrajectory::write(vsg::Output& output) const
{
    Node::write(output);

    std::string name;
    traj->getValue(META_NAME, name);
    output.write("trajName", name);

    //output.write("files", files);
}

