#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <vsg/utils/Builder.h>
#include <QObject>
#include <QSet>
//#include "vehicle-controller.h"
#include <vsg/maths/transform.h>
#include "splines/uniform_cr_spline.h"
#include "utils/arclength.h"
#include "utils/splineinverter.h"

namespace simulator {
    class Bogie;
}

namespace route
{

    class Trajectory;
    class SceneModel;
    class SplinePoint;
    class SceneObject;
    class SceneTrajectory;

    using InterpolationSpline = UniformCRSpline<vsg::dvec3, double>;

    struct InterpolatedPTCM : public InterpolationSpline::InterpolatedPTC
    {
        InterpolatedPTCM(InterpolatedPTC &&ptc) : InterpolatedPTC(std::move(ptc))
        {
            auto rot = vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), vsg::normalize(ptc.tangent)));
            auto pos = vsg::translate(ptc.position);
            calculated = pos * rot;
        }

        InterpolatedPTCM(InterpolatedPTCM&& ptcm) : InterpolatedPTC(std::move(ptcm)), calculated(std::move(ptcm.calculated)) {}

        InterpolatedPTCM& operator=(InterpolatedPTCM&& x)
        {
            InterpolatedPTC::operator=(std::move(x));
            calculated = std::move(x.calculated);
            return *this;
        }

        //InterpolatedPTCM(const InterpolatedPTCM& ptcm) : InterpolatedPTC(ptcm), calculated(ptcm.calculated) {}

        InterpolatedPTCM() :InterpolationSpline::InterpolatedPTC() {}

        vsg::dmat4 calculated;
    };

    inline bool operator==(const InterpolatedPTCM& left, const InterpolatedPTCM& right)
    {
        auto distance = vsg::length(vsg::normalize(left.tangent) - vsg::normalize(right.tangent));
        return distance < 0.1;
    }

    class Trajectory : public vsg::Inherit<vsg::Group, Trajectory>
    {
    public:

        explicit Trajectory(std::string name) { setValue(META_NAME, name); }
        Trajectory() {}

        virtual ~Trajectory() {}

        //void read(vsg::Input& input) override;
        //void write(vsg::Output& output) const override;

        virtual vsg::dvec3 getPosition(double x) const = 0;

        virtual double invert(const vsg::dvec3 vec) const = 0;

        virtual vsg::dmat4 getMatrixAt(double x) const = 0;

        virtual double getLength() const = 0;

        virtual bool isFrontReversed() const { return frontReversed; }
        virtual bool isBackReversed() const { return backReversed; }

        virtual Trajectory* getFwd() const { return fwdTraj; }
        virtual Trajectory* getBwd() const  { return bwdTraj; }

        void setBusy(simulator::Bogie *vehicle) { vehicles_on_traj.insert(vehicle); }

        void removeBusy(simulator::Bogie *vehicle) { vehicles_on_traj.remove(vehicle); }

        bool isBusy() const { return !vehicles_on_traj.isEmpty(); }

        QSet<simulator::Bogie *> getTrajVehicleSet() const { return vehicles_on_traj; }


    protected:
        QSet<simulator::Bogie *> vehicles_on_traj;

        SceneTrajectory *objects;

        Trajectory       *fwdTraj;
        Trajectory       *bwdTraj;

        bool frontReversed;
        bool backReversed;
    };

    class SplineTrajectory : public vsg::Inherit<Trajectory, SplineTrajectory>
    {
    public:

        explicit SplineTrajectory(std::string name, vsg::ref_ptr<vsg::Builder> builder,
                                  std::vector<vsg::ref_ptr<SplinePoint>> points,
                                  std::vector<vsg::vec3> geometry,
                                  vsg::ref_ptr<vsg::Node> sleeper, double distance);
        SplineTrajectory();

        virtual ~SplineTrajectory();

        vsg::dvec3 getPosition(double x) const override;

        double invert(const vsg::dvec3 vec) const override;

        vsg::dmat4 getMatrixAt(double x) const override;

        double getLength() const override { return _railSpline->totalLength(); }

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

/*
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
*/

        //double getPoints();

        void recalculate();

        int add(vsg::dvec3 lla);
        void move(vsg::dvec3 lla, int point);

    private:
        double _sleepersDistance;

        vsg::ref_ptr<vsg::Node> generateRails(vsg::DataList list, vsg::ref_ptr<vsg::ushortArray> indices);

        std::shared_ptr<UniformCRSpline<vsg::dvec3, double>> _railSpline;

        std::vector<vsg::ref_ptr<SplinePoint>> _points; //coordinates in ecef

        //std::vector<vsg::ref_ptr<vsg::Group>> _segments;
        vsg::ref_ptr<vsg::Builder> _builder;

        std::vector<vsg::vec3> _geometry;

        vsg::ref_ptr<vsg::Node> _sleeper;

        vsg::ref_ptr<vsg::Switch> _editing;
    };

    class SceneTrajectory : public vsg::Inherit<vsg::Node, SceneTrajectory>
    {
    public:
        explicit SceneTrajectory(Trajectory *traj);
        explicit SceneTrajectory();

        virtual ~SceneTrajectory();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void traverse(vsg::Visitor& visitor) override { trajectory->accept(visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { trajectory->accept(visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { trajectory->accept(visitor); }
    /*
        std::vector<std::string> files;

        std::vector<vsg::ref_ptr<vsg::MatrixTransform>> tracks;
    */
        Trajectory *trajectory;
    };
}

#endif // TRAJ_H
