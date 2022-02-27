#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/maths/transform.h>
#include <vsg/nodes/Transform.h>
#include <vsg/utils/Builder.h>
#include <vsg/commands/CopyAndReleaseBuffer.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/io/Options.h>

class RailsPointEditor;
class PointsModel;

namespace route
{
    class Trajectory;
    class SplineTrajectory;
    class Signal;

    class SceneObject : public vsg::Inherit<vsg::Transform, SceneObject>
    {
    public:
        SceneObject();
        SceneObject(vsg::ref_ptr<vsg::Node> box,
                    const vsg::dvec3 &pos,
                    const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0},
                    const vsg::dmat4 &ltw = {});
        SceneObject(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box,
                    const vsg::dvec3 &pos = {},
                    const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0},
                    const vsg::dmat4 &ltw = {});


        virtual ~SceneObject();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        vsg::dmat4 transform(const vsg::dmat4& m) const override;

        //void setRotation(const vsg::dquat &q);
        virtual void setPosition(const vsg::dvec3& pos) { _position = pos; }
        virtual void setRotation(const vsg::dquat& rot) { _quat = rot; }

        void recalculateWireframe();
        void setSelection(bool selected) { _selected = selected; }

        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Group::traverse(visitor);
            if(_selected)
                _wireframe->accept(visitor);
        }

        vsg::dvec3 getPosition() const { return _position; }
        vsg::dvec3 getWorldPosition() const { return localToWorld * _position; }
        vsg::dquat getWorldQuat() const { return _world_quat; }
        vsg::dquat getRotation() const { return _quat; }
        vsg::dquat getWorldRotation() const;

        bool isSelected() const { return _wireframe.valid(); }

        vsg::dmat4 localToWorld = {};

    protected:
        vsg::dvec3 _position = {};
        vsg::dquat _quat = {0.0, 0.0, 0.0, 1.0};

        vsg::ref_ptr<vsg::MatrixTransform> _wireframe = vsg::MatrixTransform::create();
        bool _selected;

        vsg::dquat _world_quat = {};
    };

    class SingleLoader : public vsg::Inherit<SceneObject, SingleLoader>
    {
    public:
        SingleLoader(vsg::ref_ptr<vsg::Node> loaded,
                     vsg::ref_ptr<vsg::Node> box,
                     const std::string &in_file,
                     const vsg::dvec3 &pos = {},
                     const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0},
                     const vsg::dmat4 &wtl = {});
        SingleLoader();

        virtual ~SingleLoader();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        std::string file;
    };

    enum Mask : uint64_t
    {
        Tiles = 0b1,
        SceneObjects = 0b10,
        Points = 0b100,
        Letters = 0b1000,
        Tracks = 0b10000
    };

    /*
    constexpr const std::type_info& getType(Mask type)
    {
        switch (type) {
        case Tiles:
            return typeid (vsg::Switch);
        case  SceneObjects:
            return typeid (SceneObject);
        case  Points:
            return typeid (SceneObject);
        case  Letters:
            return typeid (vsg::Switch);
        case Tracks:
            return typeid (SceneTrajectory);
        }
    }*/

    class TerrainPoint : public vsg::Inherit<SceneObject, TerrainPoint>
    {
    public:
        explicit TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                              vsg::ref_ptr<vsg::BufferInfo> buffer,
                              const vsg::dmat4 &wtl,
                              vsg::ref_ptr<vsg::Node> compiled,
                              vsg::ref_ptr<vsg::Node> box,
                              vsg::stride_iterator<vsg::vec3> point);

        virtual ~TerrainPoint();

        void setPosition(const vsg::dvec3& position) override;

    private:
        vsg::dmat4 _worldToLocal;

        vsg::ref_ptr<vsg::BufferInfo> _info;
        vsg::ref_ptr<vsg::CopyAndReleaseBuffer> _copyBufferCmd;
        vsg::stride_iterator<vsg::vec3> _vertex;
    };

    class RailPoint : public vsg::Inherit<SceneObject, RailPoint>
    {
    public:
        RailPoint(vsg::ref_ptr<vsg::Node> loaded,
                  vsg::ref_ptr<vsg::Node> box,
                  const vsg::dvec3 &pos);
        RailPoint();

        virtual ~RailPoint();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setPosition(const vsg::dvec3& position) override;
        void setRotation(const vsg::dquat& rotation) override;

        virtual void recalculate();

        vsg::dvec3 getTangent() const;

        vsg::dquat getTilt() const;

        void setTangent(double t) { _tangent = t; recalculate(); }

        Trajectory *trajectory = nullptr;

        void setTilt(double t) { _tilt = t; recalculate(); }

        void setCHeight(double h) { _cheight = h; recalculate(); }

    protected:
        double _tangent = 20.0;
        double _tilt = 0.0;
        double _cheight = 0.0;

        friend class ::RailsPointEditor;
        friend class ::PointsModel;
    };

    enum Code
    {
        W,
        R,
        YR,
        Y,
        G,
        CodeCount
    };

    enum State
    {
        OPENED,
        PREPARE_CLOSED,
        CLOSED,
        RESTR,
        PREPARE_PREAPRE,
        PREPARE_RESTR
    };

    class RailConnector : public QObject, public vsg::Inherit<RailPoint, RailConnector>
    {
        Q_OBJECT
    public:
        RailConnector(vsg::ref_ptr<vsg::Node> loaded,
                      vsg::ref_ptr<vsg::Node> box,
                      const vsg::dvec3 &pos);
        RailConnector();

        virtual ~RailConnector();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void recalculate() override;

        std::pair<Trajectory*, bool> getFwd(const Trajectory *caller) const;

        std::pair<Trajectory*, bool> getBwd(const Trajectory *caller) const;

        virtual void setFwd(Trajectory *caller);

        void setBwd(Trajectory *caller);

        void setFwdNull(Trajectory *caller);

        void setBwdNull(Trajectory *caller);

        bool isFree() const;

        void setSignal(vsg::ref_ptr<Signal> signal);

        void setReverseSignal(vsg::ref_ptr<Signal> signal);

        Trajectory *fwdTrajectory = nullptr;

        void traverse(vsg::Visitor& visitor) override;
        void traverse(vsg::ConstVisitor& visitor) const override;
        void traverse(vsg::RecordTraversal& visitor) const override;

        vsg::ref_ptr<Signal> fwdSignal() const;

        vsg::ref_ptr<Signal> bwdSignal() const;

    public slots:
        virtual void receiveBwdDirState(route::State state);
        virtual void receiveFwdDirState(route::State state);

        virtual void receiveFwdDirRef(int c);
        virtual void receiveBwdDirRef(int c);
        //void receiveFwdDirUnref(int c);
        //void receiveBwdDirUnref(int c);

    signals:
        void sendFwdCode(route::Code code);
        void sendBwdCode(route::Code code);

        void sendFwdState(route::State state);
        void sendBwdState(route::State state);

        void sendFwdRef(int c);
        void sendBwdRef(int c);
        //void sendFwdUnref();
        //void sendBwdUnref();

    protected:
        bool _reverser = false;

        vsg::ref_ptr<Signal> _fwdSignal;
        vsg::ref_ptr<Signal> _bwdSignal;
    };

    class StaticConnector : public vsg::Inherit<RailConnector, StaticConnector>
    {
        Q_OBJECT
    public:
        StaticConnector(vsg::ref_ptr<vsg::Node> loaded,
                        vsg::ref_ptr<vsg::Node> box,
                        const vsg::dvec3 &pos);
        StaticConnector();

        virtual ~StaticConnector();

        void setPosition(const vsg::dvec3& position) override;
        void setRotation(const vsg::dquat& rotation) override;
    };

    class SwitchConnector : public vsg::Inherit<StaticConnector, SwitchConnector>
    {
        Q_OBJECT
    public:
        SwitchConnector(vsg::ref_ptr<vsg::Node> loaded,
                        vsg::ref_ptr<vsg::Node> box,
                        const vsg::dvec3 &pos);
        SwitchConnector();

        virtual ~SwitchConnector();

        void setFwd(Trajectory *caller) override;

        //std::pair<Trajectory*, bool> getFwd(const Trajectory *caller) const override;

        //std::pair<Trajectory*, bool> getBwd(const Trajectory *caller) const override;

        void switchState(bool state);

        Trajectory *sideTrajectory = nullptr;

    public slots:
        void receiveBwdSideDirRef(int c);
        void receiveBwdSideDirState(route::State state);

        void receiveBwdDirState(route::State state) override;
        void receiveFwdDirState(route::State state) override;

        void receiveFwdDirRef(int c) override;
        void receiveBwdDirRef(int c) override;

    signals:
        void sendFwdSideCode(route::Code code);

        void sendFwdSideState(route::State state);

        void sendFwdSideRef(int c);

    private:
        bool _state;

        vsg::ref_ptr<route::Signal> _sideCounter;
    };
}

EVSG_type_name(route::SceneObject);
EVSG_type_name(route::SingleLoader);
EVSG_type_name(route::RailPoint);
EVSG_type_name(route::RailConnector);
EVSG_type_name(route::StaticConnector);

#endif // SCENEOBJECTS_H
