#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <vsg/utils/Builder.h>
#include <QObject>
#include <QSet>
#include "sceneobjects.h"
#include <vsg/maths/transform.h>
#include "splines/uniform_cr_spline.h"
#include "splines/cubic_hermite_spline.h"
#include "utils/arclength.h"
#include "utils/splineinverter.h"

namespace simulator {
    class Bogie;
}

namespace route
{

    class Trajectory;
    class SceneModel;
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

        virtual void recalculate() = 0;

        virtual bool isFrontReversed() const { return _frontReversed; }
        virtual bool isBackReversed() const { return _backReversed; }

        virtual Trajectory* getFwd() const { return _fwdTraj; }
        virtual Trajectory* getBwd() const  { return _bwdTraj; }

        void setBusy(simulator::Bogie *vehicle) { _vehicles_on_traj.insert(vehicle); }

        void removeBusy(simulator::Bogie *vehicle) { _vehicles_on_traj.remove(vehicle); }

        bool isBusy() const { return !_vehicles_on_traj.isEmpty(); }

        QSet<simulator::Bogie *> getTrajVehicleSet() const { return _vehicles_on_traj; }

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            for (auto& child : node._points) child->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Group::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Group::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Group::traverse(visitor);
            _track->accept(visitor);
            t_traverse(*this, visitor);
        }

    protected:
        QSet<simulator::Bogie *> _vehicles_on_traj;

        vsg::ref_ptr<vsg::Group> _track;

        std::vector<vsg::ref_ptr<SplinePoint>> _points;

        Trajectory      *_fwdTraj;
        Trajectory      *_bwdTraj;

        bool _frontReversed;
        bool _backReversed;
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

        void recalculate() override;

        int add(vsg::dvec3 lla);
        void move(vsg::dvec3 lla, int point);

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            for (auto& child : node._autoPositioned) child->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Trajectory::traverse(visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Trajectory::traverse(visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { Trajectory::traverse(visitor); t_traverse(*this, visitor); }

    private:

        void assignRails(vsg::DataList list, vsg::ref_ptr<vsg::ushortArray> indices);

        double _sleepersDistance;

        std::shared_ptr<CubicHermiteSpline<vsg::dvec3, double>> _railSpline;

        std::vector<vsg::ref_ptr<route::SceneObject>> _autoPositioned;

        vsg::ref_ptr<vsg::Builder> _builder;

        std::vector<vsg::vec3> _geometry;

        vsg::ref_ptr<vsg::Node> _sleeper;

        vsg::ref_ptr<SplinePoint>     _fwdPoint;
        vsg::ref_ptr<SplinePoint>     _bwdPoint;
    };

    class SceneTrajectory : public vsg::Inherit<vsg::Group, SceneTrajectory>
    {
    public:
        explicit SceneTrajectory(Trajectory *traj);
        explicit SceneTrajectory();

        virtual ~SceneTrajectory();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;
    };
}

#endif // TRAJ_H
