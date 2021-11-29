#ifndef TRACKOBJECTS_H
#define TRACKOBJECTS_H

#include <vsg/nodes/Transform.h>

class Track : public vsg::Inherit<vsg::Object, Track>
{
public:
    explicit Track();

    virtual ~Track();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    virtual vsg::dmat4 transform(double coord) const = 0;
    //virtual vsg::dquat rotation(double coord) const = 0;

    double lenght;

    //vsg::dmat4 ltw;
};

class StraitTrack : public vsg::Inherit<Track, StraitTrack>
{
public:
    explicit StraitTrack();

    virtual ~StraitTrack();

    vsg::dmat4 transform(double coord) const override;
    //vsg::dquat rotation(double coord) const override;
};

class CurvedTrack : public vsg::Inherit<Track, CurvedTrack>
{
public:
    explicit CurvedTrack();

    virtual ~CurvedTrack();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(double coord) const override;
    //vsg::dquat rotation(double coord) const override;

    double radius;
};

#endif