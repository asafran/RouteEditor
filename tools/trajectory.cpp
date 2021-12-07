#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <vsg/io/read.h>

TrackSection::TrackSection(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4 &in_matrix)
    : matrix(in_matrix)
    , filename(in_file)
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

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}

void TrackSection::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("filename", filename);

    output.write("incl", inclination);
    output.write("matrix", matrix);
    output.write("filename", filename);

    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Trajectory::Trajectory()
  : vsg::Inherit<vsg::Object, Trajectory>()
  , lenght(0.0)
  , frontReversed()
  , sections()
{

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Trajectory::~Trajectory()
{

}

void Trajectory::read(vsg::Input& input)
{
    Object::read(input);


    input.read("sections", sections);
    input.read("lenght", lenght);
    input.read("next", next);
    input.read("frontReversed", lenght);
}

void Trajectory::write(vsg::Output& output) const
{
    Object::write(output);

}

void Trajectory::addTrack(vsg::ref_ptr<vsg::Node> node, const std::string &name)
{
    auto loader = TrackSection::create(node, name, matrixStack.back());

    matrixStack.emplace_back(matrixStack.back() * loader->track->transform(loader->track->lenght));
    lenght += loader->track->lenght;

    //track->ltw = vsg::inverse(vsg::translate(position)) * vsg::inverse(next);
    sections.push_back(loader);
}

void Trajectory::removeTrack()
{
    sections.pop_back();
}

void Trajectory::recalculatePositions()
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
vsg::dmat4 Trajectory::getPosition(double x) const
{
    auto section = getSection(x);

    return (*section.first)->world(section.second);
}

std::pair<Sections::const_iterator, double> Trajectory::getSection(double x) const
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

