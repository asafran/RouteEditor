#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <QObject>
#include <QSet>
#include "trackobjects.h"
//#include "vehicle-controller.h"
#include <vsg/maths/transform.h>

class Bogie;

class TrackSection : public vsg::Inherit<vsg::Transform, TrackSection>
{
public:
    TrackSection() {}
    TrackSection(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4 &in_matrix);

    virtual ~TrackSection() {}

    vsg::dmat4 world(double coord) const { return track->transform(coord) * vsg::inverse(matrix); }

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(const vsg::dmat4& m) const override { return m * matrix; }

    vsg::ref_ptr<Track> track;
    double inclination = 0.0;
    vsg::dmat4 matrix = {};
    std::string filename = "";
};

using Sections = std::vector<vsg::ref_ptr<TrackSection>>;

class Trajectory : public vsg::Inherit<vsg::Object, Trajectory>
{
public:

    Trajectory();

    ~Trajectory();

    virtual vsg::dmat4 getPosition(double x) const ;

    virtual std::pair<Sections::const_iterator, double> getSection(double x) const;

    virtual double getLength() const { return lenght; }

    virtual bool isFrontReversed() const { return frontReversed; }

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    void recalculatePositions();

    void addTrack(vsg::ref_ptr<vsg::Node> node, const std::string &name);
    void removeTrack();

    void setBusy(Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

    void removeBusy(Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

    bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

    QSet<Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }

    //TrackSection *getSectionPtr(size_t sec);

    virtual Sections::const_iterator getBegin() const { return sections.cbegin(); }
    virtual Sections::const_iterator getEnd() const { return sections.cend(); }
    virtual int size() const { return sections.size(); }

    std::string next;
    std::string name;

    Trajectory       *fwdTraj;
    Trajectory       *bwdTraj;

protected:
    QSet<Bogie *> vehicles_on_traj;

    Sections sections;

    double lenght;
    bool frontReversed;

    std::vector<vsg::dmat4> matrixStack;
};
/*
class Junction
{
public:

    explicit Trajectory(Sections in_sections, std::string in_name);

    ~Trajectory();

    virtual vsg::dmat4 getPosition(double x) const ;

    virtual std::pair<Sections::const_iterator, double> getSection(double x) const;

    virtual double getLength() const { return length; }

    void setBusy(Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

    void removeBusy(Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

    bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

    QSet<Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }

    //TrackSection *getSectionPtr(size_t sec);

    const std::string         name;
    const double          length;

    virtual Sections::const_iterator getBegin() const { return sections.cbegin(); }
    virtual Sections::const_iterator getEnd() const { return sections.cend(); }

    const bool frontReversed;
    Trajectory       *fwdTraj;
    Trajectory       *bwdTraj;

protected:
    QSet<Bogie *> vehicles_on_traj;

    const Sections sections;

};
*/
#endif // TRAJ_H
