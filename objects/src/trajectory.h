#ifndef TRAJ_H
#define TRAJ_H

#include <vsg/maths/mat4.h>
#include <vsg/utils/Builder.h>
#include <QObject>
#include <QSet>
#include "sceneobjects.h"
#include <vsg/maths/transform.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/AbsoluteTransform.h>
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
    class Vehicle;
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


    class Trajectory : public QObject, public vsg::Inherit<vsg::Group, Trajectory>
    {
        Q_OBJECT
    public:

        Trajectory(std::string name, vsg::ref_ptr<RailConnector> bwdPoint, vsg::ref_ptr<RailConnector> fwdPoint);
        Trajectory();

        virtual ~Trajectory();

        //void read(vsg::Input& input) override;
        //void write(vsg::Output& output) const override;

        virtual vsg::dvec3 getCoordinate(double x) const = 0;

        virtual double invert(const vsg::dvec3 vec) const = 0;

        virtual vsg::dmat4 getMatrixAt(double x) const = 0;

        virtual vsg::dmat4 getLocalMatrixAt(double x) const = 0;

        virtual double getLength() const = 0;

        virtual void recalculate() = 0;

        void updateAttached();

        void detatch();

        std::pair<Trajectory*, bool> getFwd() const { return _fwdPoint->getFwd(this); }
        std::pair<Trajectory*, bool> getBwd() const { return _bwdPoint->getBwd(this); }

        vsg::ref_ptr<RailConnector> getFwdPoint() const { return _fwdPoint; }
        vsg::ref_ptr<RailConnector> getBwdPoint() const { return _bwdPoint; }

        void connectSignalling();

        void setBusy() { emit sendRef(); }//_vehicles_on_traj.insert(vehicle); }

        void removeBusy(simulator::Vehicle *vehicle) { _vehicles_on_traj.remove(vehicle); }

        bool isBusy() const { return !_vehicles_on_traj.isEmpty(); }
/*
        bool isBlockBusy(bool fwd) const
        {
            if(isBusy())
                return true;
            auto [ trj, reversed, isolated ] = fwd ? getFwd() : getBwd();
            if(trj && !isolated)
                return trj->isBlockBusy(reversed);
            return false;
        }*/

        QSet<simulator::Vehicle *> getTrajVehicleSet() const { return _vehicles_on_traj; }
/*
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Group::traverse(visitor);
            _track->accept(visitor);
        }*/

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            node._fwdPoint->accept(visitor);
            node._bwdPoint->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Group::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Group::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override { Group::traverse(visitor); t_traverse(*this, visitor); }

    signals:
        void sendFwdCode(route::Code code);
        void sendBwdCode(route::Code code);

        void sendRef();
        void sendUnref();

        void transmitFwdCode(route::Code code);
        void transmitBwdCode(route::Code code);

        void ref();
        void unref();

        void update();

    protected:

        QSet<simulator::Vehicle *> _vehicles_on_traj;

        vsg::ref_ptr<RailConnector>     _fwdPoint;
        vsg::ref_ptr<RailConnector>     _bwdPoint;

        route::Code _fwdCode;
        route::Code _bwdCode;
    };

    class SplineTrajectory : public vsg::Inherit<Trajectory, SplineTrajectory>
    {
        Q_OBJECT
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

        vsg::dmat4 getLocalMatrixAt(double x) const override { return getMatrixAt(x); }

        double getLength() const override { return _railSpline->totalLength(); }

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void recalculate() override;

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
            node._track->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Trajectory::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Trajectory::traverse(visitor); t_traverse(*this, visitor); }
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

        friend class Topology;

        friend class ::PointsModel;

        friend class ::AddRails;
    };

    class PointsTrajectory : public vsg::Inherit<Trajectory, PointsTrajectory>
    {
        Q_OBJECT
    public:
        PointsTrajectory(std::string name,
                         vsg::ref_ptr<RailConnector> bwdPoint,
                         vsg::ref_ptr<RailConnector> fwdPoint,
                         vsg::ref_ptr<vsg::AnimationPath> path);
        PointsTrajectory();

        virtual ~PointsTrajectory();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        vsg::dvec3 getCoordinate(double x) const override;

        double invert(const vsg::dvec3 vec) const override;

        vsg::dmat4 getMatrixAt(double x) const override;

        vsg::dmat4 getLocalMatrixAt(double x) const override;

        double getLength() const override { return _path->locations.rbegin()->first; }

        void recalculate() override {};

        vsg::dmat4 localToWorld = {};

    protected:
        vsg::ref_ptr<vsg::AnimationPath> _path;

    };

    class Junction : public vsg::Inherit<SceneObject, Junction>
    {
    public:
        Junction(std::string name,
                 vsg::ref_ptr<vsg::AnimationPath> strait,
                 vsg::ref_ptr<vsg::AnimationPath> side,
                 vsg::ref_ptr<vsg::AnimationPath> switcherPath, vsg::ref_ptr<vsg::Node> mrk,
                 vsg::ref_ptr<vsg::Node> box,
                 vsg::ref_ptr<vsg::Node> rails,
                 vsg::ref_ptr<vsg::MatrixTransform> switcher);
        Junction();

        virtual ~Junction();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setPosition(const vsg::dvec3& pos) override;
        void setRotation(const vsg::dquat& rot) override;

        template<class N, class V>
        static void t_traverse(N& node, V& visitor)
        {
            node._switcher->accept(visitor);
            node._strait->accept(visitor);
            node._side->accept(visitor);
        }

        void traverse(vsg::Visitor& visitor) override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::ConstVisitor& visitor) const override { Transform::traverse(visitor); t_traverse(*this, visitor); }
        void traverse(vsg::RecordTraversal& visitor) const override
        {
            SceneObject::traverse(visitor);
            _switcher->accept(visitor);
            t_traverse(*this, visitor);
        }

        void setState(bool state) { _state = state; }

    private:
        vsg::ref_ptr<vsg::MatrixTransform> _switcher;

        vsg::ref_ptr<PointsTrajectory>     _strait;
        vsg::ref_ptr<PointsTrajectory>     _side;
        vsg::ref_ptr<RailConnector>     _switcherPoint;

        bool _state; //true if switched
    };
}

EVSG_type_name(route::SplineTrajectory);
EVSG_type_name(route::SceneTrajectory);
EVSG_type_name(route::Junction);

#endif // TRAJ_H
