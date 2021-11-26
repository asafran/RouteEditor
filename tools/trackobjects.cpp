#include "trackobjects.h"
#include <vsg/maths/transform.h>

Track::Track()
    : vsg::Inherit<vsg::Object, Track>()
{
}
Track::~Track() {}

void Track::read(vsg::Input& input)
{
    Object::read(input);

    input.read("lenght", lenght);
    //input.read("next", next);
}

void Track::write(vsg::Output& output) const
{
    Object::write(output);

    output.write("lenght", lenght);
    //output.write("next", next);
}

StraitTrack::StraitTrack()
    : vsg::Inherit<Track, StraitTrack>()
{
}
StraitTrack::~StraitTrack() {}

vsg::dmat4 StraitTrack::transform(double coord) const
{
    return vsg::dmat4(1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, 0, 1, 0,
                      0, coord, 0, 1);
}

CurvedTrack::CurvedTrack()
    : vsg::Inherit<Track, CurvedTrack>()
{
}
CurvedTrack::~CurvedTrack() {}

void CurvedTrack::read(vsg::Input& input)
{
    Track::read(input);

    input.read("radius", radius);
}

void CurvedTrack::write(vsg::Output& output) const
{
    Track::write(output);

    output.write("radius", radius);
}

vsg::dmat4 CurvedTrack::transform(double coord) const
{
    auto angle = coord / radius;
    auto matrix = vsg::rotate(-angle, 0.0, 0.0, 1.0);
    matrix[3][0] = -radius * (cos(angle) - 1);
    matrix[3][1] = radius * sin(angle);
    return matrix;
}
