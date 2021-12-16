#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <QObject>
#include <QSet>
#include "trackobjects.h"
#include "sceneobjects.h"
//#include "vehicle-controller.h"
#include <vsg/maths/transform.h>
#include "splines/uniform_cr_spline.h"
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/Constants.hpp>

class Bogie;
class Trajectory;
class SceneModel;

/*
class TrackSection : public vsg::Inherit<vsg::Transform, TrackSection>
{
public:
    TrackSection() {}
    TrackSection(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, vsg::dmat4 in_mat, Trajectory* parent);

    virtual ~TrackSection() {}

    vsg::dmat4 world(double coord) const { return transform(track->transform(coord)); }

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(const vsg::dmat4& m) const override;

    vsg::ref_ptr<Track> track;
    double inclination = 0.0;
    vsg::dmat4 matrix = {};
    std::string filename = "";

    TrackSection *prev;

    SectionTrajectory *traj;
};
*/

//using Sections = std::vector<vsg::ref_ptr<TrackSection>>;

class Trajectory : public vsg::Inherit<vsg::Node, Trajectory> //subclass from Node to implement junction
{
public:

    explicit Trajectory(std::string name) { setValue(META_NAME, name); }
    Trajectory() {}

    ~Trajectory() {}

    virtual vsg::dvec3 getPosition(double x) const = 0;

    virtual vsg::dmat4 getMatrixAt(double x) const = 0;

    virtual double getLength() const = 0;

    virtual bool isFrontReversed() const { return frontReversed; }
    virtual bool isBackReversed() const { return backReversed; }

    virtual Trajectory* getFwd() const { return fwdTraj; }
    virtual Trajectory* getBwd() const  { return bwdTraj; }

    void setBusy(Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

    void removeBusy(Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

    bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

    QSet<Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }


protected:
    QSet<Bogie *> vehicles_on_traj;

    Trajectory       *fwdTraj;
    Trajectory       *bwdTraj;

    bool frontReversed;
    bool backReversed;
};

class SplineTrajectory : public vsg::Inherit<Trajectory, SectionTrajectory>
{
public:

    explicit SplineTrajectory(std::string name, const vsg::dvec3 &lla_point);
    SplineTrajectory();

    ~SplineTrajectory();

    vsg::dvec3 getPosition(double x) const override;

    vsg::dmat4 getMatrixAt(double x) const override;

    double getLength() const override;

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    template<class N, class V>
    static void t_traverse(N& node, V& visitor)
    {
        for (auto it = node.sleepers.begin(); it != node.sleepers.end(); ++it)
            (*it)->accept(visitor);
        node.rails->accept(visitor);
    }

    void traverse(vsg::Visitor& visitor) override { t_traverse(*this, visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { t_traverse(*this, visitor); }

    void recalculate();

    int add(vsg::dvec3 lla);
    void move(vsg::dvec3 lla, int point);

private:

    void applyChanges(vsg::ref_ptr<vsg::vec3Array> vertices,
                      vsg::ref_ptr<vsg::vec3Array> colors,
                      vsg::ref_ptr<vsg::vec2Array> texcoords,
                      vsg::ref_ptr<vsg::ushortArray> indices);

    std::shared_ptr<UniformCRSpline<vsg::dvec2>> railSpline; //coordinates in lla
    std::shared_ptr<UniformCRSpline<vsg::dvec2>> elevationSpline; //elevation in meters

    vsg::ref_ptr<vsg::Commands> rails;
    std::vector<vsg::ref_ptr<vsg::MatrixTransform>> sleepers;
};

class SceneTrajectory : public vsg::Inherit<vsg::Node, SceneTrajectory>
{
public:
    explicit SceneTrajectory(Trajectory *trajectory);
    explicit SceneTrajectory();

    virtual ~SceneTrajectory();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    void traverse(vsg::Visitor& visitor) override { traj->accept(visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { traj->accept(visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { traj->accept(visitor); }
/*
    std::vector<std::string> files;

    std::vector<vsg::ref_ptr<vsg::MatrixTransform>> tracks;
*/
    Trajectory *traj;
};

#endif // TRAJ_H
