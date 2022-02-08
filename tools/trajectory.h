#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <vsg/utils/Builder.h>
#include <QObject>
#include <QSet>
#include "sceneobjects.h"
#include <vsg/maths/transform.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/utils/AnimationPath.h>
#include "splines/uniform_cr_spline.h"
#include "splines/cubic_hermite_spline.h"
#include "utils/arclength.h"
#include "utils/splineinverter.h"

#include "tiny_obj_loader.h"

namespace simulator {
    class Bogie;
}

namespace route
{

    class Trajectory;
    class SceneModel;
    class SceneTrajectory;

    using InterpolationSpline = CubicHermiteSpline<vsg::dvec3, double>;

    struct InterpolatedPTM : public InterpolationSpline::InterpolatedPT
    {
        InterpolatedPTM(InterpolatedPT &&pt, const vsg::dvec3 &offset = {}) : InterpolatedPT(std::move(pt))
        {
            auto norm = vsg::normalize(pt.position);
            vsg::dquat w_quat(vsg::dvec3(0.0, 1.0, 0.0), norm);

            auto t = vsg::inverse(vsg::rotate(w_quat)) * pt.tangent;
            auto cos = vsg::dot(vsg::normalize(vsg::dvec2(t.x, t.z)), vsg::dvec2(0.0, 1.0));

            double angle = vsg::PI;
            if(t.x < 0)
                angle -= std::acos(cos);
            else
                angle += std::acos(cos);

            vsg::dquat t_quat(angle, vsg::dvec3(0.0, 1.0, 0.0));

            auto rot = vsg::rotate(mult(w_quat, t_quat));
            auto pos = vsg::translate(pt.position - offset);
            calculated = pos * rot;
        }

        InterpolatedPTM(InterpolatedPTM&& ptm) : InterpolatedPT(std::move(ptm)), calculated(std::move(ptm.calculated)) {}

        InterpolatedPTM& operator=(InterpolatedPTM&& x)
        {
            InterpolatedPT::operator=(std::move(x));
            calculated = std::move(x.calculated);
            return *this;
        }

        //InterpolatedPTCM(const InterpolatedPTCM& ptcm) : InterpolatedPTC(ptcm), calculated(ptcm.calculated) {}

        InterpolatedPTM() :InterpolationSpline::InterpolatedPT() {}

        vsg::dmat4 calculated;
    };

    inline bool operator==(const InterpolatedPTM& left, const InterpolatedPTM& right)
    {
        auto distance = vsg::length(vsg::normalize(left.tangent) - vsg::normalize(right.tangent));
        return distance < 0.1;
    }


    class Trajectory : public vsg::Inherit<SceneObject, Trajectory>
    {
    public:

        explicit Trajectory(std::string name) : vsg::Inherit<SceneObject, Trajectory>() { setValue(META_NAME, name); }
        Trajectory() {}

        virtual ~Trajectory() {}

        //void read(vsg::Input& input) override;
        //void write(vsg::Output& output) const override;

        virtual vsg::dvec3 getCoordinate(double x) const = 0;

        virtual double invert(const vsg::dvec3 vec) const = 0;

        virtual vsg::dmat4 getMatrixAt(double x) const = 0;

        virtual double getLength() const = 0;

        virtual void recalculate() = 0;

        virtual std::pair<Trajectory*, bool> getFwd() const = 0;
        virtual std::pair<Trajectory*, bool> getBwd() const = 0;

        void setBusy(simulator::Bogie *vehicle) { _vehicles_on_traj.insert(vehicle); }

        void removeBusy(simulator::Bogie *vehicle) { _vehicles_on_traj.remove(vehicle); }

        bool isBusy() const { return !_vehicles_on_traj.isEmpty(); }

        QSet<simulator::Bogie *> getTrajVehicleSet() const { return _vehicles_on_traj; }
/*
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Group::traverse(visitor);
            _track->accept(visitor);
        }*/

    protected:
        QSet<simulator::Bogie *> _vehicles_on_traj;
    };

    class SplineTrajectory : public vsg::Inherit<Trajectory, SplineTrajectory>
    {
    public:

        SplineTrajectory(std::string name,
                         vsg::ref_ptr<RailConnector> bwdPoint,
                         vsg::ref_ptr<RailConnector> fwdPoint,
                         vsg::ref_ptr<vsg::Builder> builder,
                         //vsg::ref_ptr<Compiler> compiler,
                         tinyobj::attrib_t rail,
                         vsg::ref_ptr<vsg::Data> texture,
                         vsg::ref_ptr<vsg::Node> sleeper, double distance, double gaudge);
        SplineTrajectory();

        virtual ~SplineTrajectory();

        vsg::dvec3 getCoordinate(double x) const override;

        double invert(const vsg::dvec3 vec) const override;

        vsg::dmat4 getMatrixAt(double x) const override;

        double getLength() const override { return _railSpline->totalLength(); }

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void recalculate() override;

        void setPosition(const vsg::dvec3& pos) override { }
        void setRotation(const vsg::dquat& rot) override { }

        std::pair<Trajectory*, bool> getFwd() const override { return _fwdPoint->getFwd(this); }
        std::pair<Trajectory*, bool> getBwd() const override { return _bwdPoint->getBwd(this); }

        void add(vsg::ref_ptr<RailPoint> rp);

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            for (auto& child : node._autoPositioned) child->accept(visitor);
            for (auto& child : node._points) child->accept(visitor);
            node._bwdPoint->accept(visitor);
            node._fwdPoint->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Trajectory::traverse(visitor);
            _track->accept(visitor);
            t_traverse(*this, visitor);
        }


    private:

        void updateSpline();

        void assignRails(std::pair<vsg::DataList, vsg::ref_ptr<vsg::ushortArray>> data);

        std::pair<vsg::DataList, vsg::ref_ptr<vsg::ushortArray>> createSingleRail(const vsg::vec3 &offset, const std::vector<InterpolatedPTM> &derivatives) const;

        void updateAttached();

        double _sleepersDistance;

        double _gaudge;

        std::shared_ptr<CubicHermiteSpline<vsg::dvec3, double>> _railSpline;

        std::vector<vsg::ref_ptr<route::SceneObject>> _autoPositioned;

        std::vector<vsg::ref_ptr<route::RailPoint>> _points;

        vsg::ref_ptr<vsg::Builder> _builder;

        vsg::ref_ptr<vsg::Group> _track;

        std::vector<vsg::vec3> _geometry;

        std::vector<vsg::vec2> _uv1;
        std::vector<vsg::vec2> _uv2;

        vsg::ref_ptr<vsg::Data> _texture;

        vsg::ref_ptr<vsg::Node> _sleeper;

        vsg::ref_ptr<RailConnector>     _fwdPoint;
        vsg::ref_ptr<RailConnector>     _bwdPoint;

        friend class Topology;
    };

    class Junction : public vsg::Inherit<Trajectory, Junction>
    {
        explicit Junction(std::string name,
                          vsg::ref_ptr<RailConnector> bwdPoint,
                          vsg::ref_ptr<RailConnector> fwdPoint,
                          vsg::ref_ptr<RailConnector> fwd2Point,
                          vsg::ref_ptr<vsg::AnimationPath> strait,
                          vsg::ref_ptr<vsg::AnimationPath> side,
                          vsg::ref_ptr<vsg::AnimationPath> switcherPath,
                          vsg::ref_ptr<vsg::Node> rails,
                          vsg::ref_ptr<vsg::MatrixTransform> switcher);
        Junction();

        virtual ~Junction();

        vsg::dvec3 getCoordinate(double x) const override;

        double invert(const vsg::dvec3 vec) const override;

        vsg::dmat4 getMatrixAt(double x) const override;

        double getLength() const override { return _state ? _side->locations.rbegin()->first : _strait->locations.rbegin()->first; }

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void recalculate() override {};

        void setPosition(const vsg::dvec3& pos) override;
        void setRotation(const vsg::dquat& rot) override;

        std::pair<Trajectory*, bool> getFwd() const override { return _state ? _fwd2Point->getFwd(this) : _fwdPoint->getFwd(this); }
        std::pair<Trajectory*, bool> getBwd() const override { return _bwdPoint->getBwd(this); }

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            node._fwdPoint->accept(visitor);
            node._fwd2Point->accept(visitor);
            node._bwdPoint->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Trajectory::traverse(visitor);
            _switcher->accept(visitor);
            t_traverse(*this, visitor);
        }

        void setState(bool state) { _state = state; }

    private:
        vsg::ref_ptr<vsg::AnimationPath> _strait;
        vsg::ref_ptr<vsg::AnimationPath> _side;

        vsg::ref_ptr<vsg::MatrixTransform> _switcher;

        RailConnector     *_fwdPoint;
        RailConnector     *_fwd2Point;
        RailConnector     *_bwdPoint;

        bool _state; //true if switched
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
