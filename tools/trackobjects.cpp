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

double StraitTrack::coord(vsg::dvec3 point) const
{
    return point.y;
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

double CurvedTrack::coord(vsg::dvec3 point) const
{
    return asin(point.y / radius) * radius;
}

LinearInterpolation::LinearInterpolation(vsg::dvec2 first, vsg::dvec2 second)
{
    auto t = second - first;
    lenght = vsg::length(t);
    orth = vsg::dvec3(t * (1 / lenght), 0.0);
    begin = vsg::dvec3(first, 0.0);
    matrix = vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 1.0, 0.0), orth));
}

LinearInterpolation::LinearInterpolation() {}

LinearInterpolation::~LinearInterpolation() {}

vsg::dmat4 LinearInterpolation::transform(double coord) const
{
    return vsg::translate(begin + orth * coord) * matrix;
}
