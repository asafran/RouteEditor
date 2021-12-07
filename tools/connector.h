#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "sceneobjects.h"

class Connector : public vsg::Inherit<vsg::Object, Connector>
{
public:

    Connector();

    virtual ~Connector();

    virtual Trajectory *getFwdTraj() const { return fwdTraj; }

    virtual Trajectory *getBwdTraj() const { return bwdTraj; }

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    bool isReversed() const { return reverser; }

protected:

    Trajectory *fwdTraj;

    Trajectory *bwdTraj;

    bool reverser;

    std::string name;
};

#endif // CONNECTOR_H
