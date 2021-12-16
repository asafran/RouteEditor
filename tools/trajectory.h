#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <QObject>
#include <QSet>
#include "trackobjects.h"
#include "sceneobjects.h"
//#include "vehicle-controller.h"
#include <vsg/maths/transform.h>

class Bogie;
class Trajectory;
class SceneModel;

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

using Sections = std::vector<vsg::ref_ptr<TrackSection>>;

class Trajectory : public vsg::Inherit<SceneObject, Trajectory> //subclass from Node to implement junction
{
public:

    explicit Trajectory(std::string name) { setValue(META_NAME, name); }
    Trajectory() {}

    ~Trajectory() {}

    virtual vsg::dmat4 getPosition(double x) const = 0;

    virtual std::pair<Sections::const_iterator, double> getSection(double x) const = 0;

    virtual double getLength() const = 0;

    virtual bool isFrontReversed() const = 0;
/*
    virtual void setOffset(const vsg::dvec3 &pos) = 0;
    virtual void setOffset(const vsg::dmat4 &offset) = 0;
*/
    virtual Trajectory* getFwd() const  = 0;

    Trajectory* getBwd() const { return trajectory; }

    void setBusy(Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

    void removeBusy(Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

    bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

    QSet<Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }

    //TrackSection *getSectionPtr(size_t sec);

    virtual Sections::const_iterator getBegin(const Trajectory*) const = 0;
    virtual Sections::const_iterator getEnd(const Trajectory*) const = 0;
    virtual int size() const = 0;

    //Trajectory       *bwdTraj;

protected:
    QSet<Bogie *> vehicles_on_traj;
};

class SectionTrajectory : public vsg::Inherit<Trajectory, SectionTrajectory>
{
public:

    explicit SectionTrajectory(std::string name, const vsg::dmat4 &offset);
    SectionTrajectory();

    ~SectionTrajectory();

    vsg::dmat4 getPosition(double x) const override;

    std::pair<Sections::const_iterator, double> getSection(double x) const override;

    double getLength() const override { return lenght; }

    bool isFrontReversed() const override { return frontReversed; }

    //void setOffset(const vsg::dvec3 &pos) override { matrixStack.front()[3] = vsg::dvec4(pos, 1.0); }
    //void setOffset(const vsg::dmat4 &offset) override { matrixStack.front() = offset; }

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    void recalculatePositions();

    void addTrack(vsg::ref_ptr<vsg::Node> node, const std::string &name);
    void removeTrack(int section);

    Sections::const_iterator getBegin(const Trajectory*) const  override { return sections.cbegin(); }
    Sections::const_iterator getEnd(const Trajectory*) const override { return sections.cend(); }
    int size() const override { return sections.size(); }

    template<class N, class V>
    static void t_traverse(N& node, V& visitor)
    {
        for (auto it = node.sections.begin(); it != node.sections.end(); ++it)
            (*it)->accept(visitor);
    }

    void accept(vsg::Visitor& visitor) override
    {
        if(auto csov = dynamic_cast<SceneObjectsVisitor*>(&visitor); csov)
            csov->apply(*this);
        else
            visitor.apply(*this);
    }
    void accept(vsg::ConstVisitor& visitor) const override
    {
        if(auto csov = dynamic_cast<ConstSceneObjectsVisitor*>(&visitor); csov)
            csov->apply(*this);
        else
            visitor.apply(*this);
    }
    void accept(vsg::RecordTraversal& visitor) const override { visitor.apply(*this); }

    void traverse(vsg::Visitor& visitor) override { t_traverse(*this, visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { t_traverse(*this, visitor); }

    Trajectory       *fwdTraj;

private:
    Sections sections;

    double lenght;
    bool frontReversed;

    std::vector<vsg::dmat4> matrixStack;
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

class Junction2 : public vsg::Inherit<Trajectory, Junction2>
{
public:

    explicit Junction2(std::string name) { setValue(META_NAME, name); }
    Junction2() {}

    ~Junction2() {}

    virtual vsg::dmat4 getPosition(double x) const;

    virtual std::pair<Sections::const_iterator, double> getSection(double x) const;

    virtual double getLength() const;

    virtual bool isFrontReversed() const;

    virtual void setOffset(const vsg::dvec3 &pos);
    virtual void setOffset(const vsg::dmat4 &offset);

    void setBusy(Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

    void removeBusy(Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

    bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

    QSet<Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }

    //TrackSection *getSectionPtr(size_t sec);

    virtual Sections::const_iterator getBegin(const Trajectory* prev) const = 0;
    virtual Sections::const_iterator getEnd(const Trajectory* prev) const = 0;
    virtual int size() const = 0;

protected:

    Sections sections1;
    double lenght1;
    bool frontReversed1;

    Sections sections2;
    double lenght2;
    bool frontReversed2;

    Trajectory       *fwdTraj1;
    Trajectory       *fwdTraj2;

    bool state = false;
};

#endif // TRAJ_H
