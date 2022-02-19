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
#include "Constants.h"
#include "tools.h"

#include "../tiny_obj_loader.h"

namespace simulator {
    class Bogie;
}

class PointsModel;
class AddRails;

namespace route
{

    class Trajectory;
    class SceneModel;
    class SceneTrajectory;
    class Topology;

    using InterpolationSpline = CubicHermiteSpline<vsg::dvec3, double>;

    struct InterpolatedPTM : public InterpolationSpline::InterpolatedPT
    {
        InterpolatedPTM(InterpolatedPT &&pt, const vsg::dquat &tilt = {0.0, 0.0, 0.0, 1.0}, const vsg::dvec3 &offset = {}) : InterpolatedPT(std::move(pt))
        {
            auto norm = vsg::normalize(pt.position);
            vsg::dquat w_quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

            auto t = vsg::inverse(vsg::rotate(w_quat)) * pt.tangent;
            auto cos = vsg::dot(vsg::normalize(vsg::dvec2(t.x, t.y)), vsg::dvec2(0.0, 1.0));

            double angle = t.x < 0 ? std::acos(cos) : -std::acos(cos);

            rot = vsg::dquat(angle, vsg::dvec3(0.0, 0.0, 1.0));

            auto result = mult(mult(w_quat, rot), tilt);
            auto pos = vsg::translate(pt.position - offset);
            calculated = pos * vsg::rotate(result);
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
        vsg::dquat rot;
        size_t index;
    };

    inline bool operator==(const InterpolatedPTM& left, const InterpolatedPTM& right)
    {
        auto distance = vsg::length(vsg::normalize(left.tangent) - vsg::normalize(right.tangent));
        return distance < 0.1;
    }


    class Trajectory : public vsg::Inherit<SceneObject, Trajectory>
    {
    public:

        explicit Trajectory(std::string name) : vsg::Inherit<SceneObject, Trajectory>() { setValue(app::NAME, name); }
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
                         std::string railPath, std::string fillPath,
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

        void setFwdPoint(RailConnector *rc);
        void setBwdPoint(RailConnector *rc);

        void add(vsg::ref_ptr<RailPoint> rp, bool autoRotate = true);
        void remove(size_t index);
        void remove(vsg::ref_ptr<RailPoint> rp);

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            for (auto& child : node._autoPositioned) child->accept(visitor);
            for (auto& child : node._points) child->accept(visitor);
            node._bwdPoint->accept(visitor);
            node._fwdPoint->accept(visitor);
            node._track->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { Trajectory::traverse(visitor); t_traverse(*this, visitor); }

        struct VertexData
        {
            VertexData(const vsg::vec3 &vert)
            {
                verticle = vert;
            }

            vsg::vec3 verticle = {};

            float uv = 0.0;

            vsg::vec3 normal = {};
        };

        void reloadData();

        void updateAttached();

        vsg::ref_ptr<RailConnector> getFwdPoint() const;

        vsg::ref_ptr<RailConnector> getBwdPoint() const;

    private:

        void updateSpline();

        void assignRails(const std::vector<InterpolatedPTM> &derivatives);

        vsg::ref_ptr<vsg::VertexIndexDraw> createGeometry(const vsg::vec3 &offset,
                                                          const std::vector<InterpolatedPTM> &derivatives,
                                                          const std::vector<VertexData> &geometry) const;

        std::pair<std::vector<VertexData>, vsg::ref_ptr<vsg::Data>> loadData(std::string path);

        vsg::ref_ptr<route::RailPoint> findFloorPoint(double t) const;

        vsg::dquat mixTilt(double T) const;

        double _sleepersDistance;

        double _gaudge;

        std::shared_ptr<CubicHermiteSpline<vsg::dvec3, double>> _railSpline;

        std::vector<vsg::ref_ptr<route::SceneObject>> _autoPositioned;

        std::vector<vsg::ref_ptr<route::RailPoint>> _points;

        vsg::ref_ptr<vsg::Builder> _builder;

        vsg::ref_ptr<vsg::MatrixTransform> _track;

        std::string _railPath;
        std::vector<VertexData> _rail;
        vsg::ref_ptr<vsg::Data> _railTexture;

        std::string _fillPath;
        std::vector<VertexData> _fill;
        vsg::ref_ptr<vsg::Data> _fillTexture;

        vsg::ref_ptr<vsg::Node> _sleeper;

        vsg::ref_ptr<RailConnector>     _fwdPoint;
        vsg::ref_ptr<RailConnector>     _bwdPoint;

        friend class Topology;

        friend class ::PointsModel;

        friend class ::AddRails;
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
}

EVSG_type_name(route::SplineTrajectory);
EVSG_type_name(route::SceneTrajectory);
EVSG_type_name(route::Junction);

#endif // TRAJ_H
