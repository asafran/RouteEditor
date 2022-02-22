#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include <vsg/io/read.h>
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/LOD.h>
#include "signal.h"
#include "topology.h"

namespace route
{
    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> box,
                             const vsg::dvec3& pos,
                             const vsg::dquat& w_quat,
                             const vsg::dmat4 &ltw)
        : vsg::Inherit<vsg::Transform, SceneObject>()
        , _position(pos)
        , _quat(0.0, 0.0, 0.0, 1.0)
        , _selected(false)
        , _world_quat(w_quat)
        , localToWorld(ltw)
    {
        _wireframe = vsg::MatrixTransform::create();
        _wireframe->addChild(box);
    }

    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded,
                             vsg::ref_ptr<vsg::Node> box,
                             const vsg::dvec3 &pos,
                             const vsg::dquat& w_quat,
                             const vsg::dmat4 &ltw)
        : SceneObject(box, pos, w_quat, ltw)
    {
        addChild(loaded);
    }

    SceneObject::SceneObject() : vsg::Inherit<vsg::Transform, SceneObject>() , _selected(false) {}

    SceneObject::~SceneObject() {}

    void SceneObject::read(vsg::Input& input)
    {
        Group::read(input);

        input.read("quat", _quat);
        input.read("world_quat", _world_quat);
        input.read("wireframe", _wireframe);
        input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        input.read("ltw", localToWorld);
        input.read("coord", _position);
    }

    void SceneObject::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("wireframe", _wireframe);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("ltw", localToWorld);

        output.write("coord", _position);
    }
    vsg::dquat SceneObject::getWorldRotation() const
    {
        return mult(_world_quat, _quat);
    }

    void SceneObject::recalculateWireframe()
    {
        vsg::ComputeBounds cb;
        t_traverse(*this, cb);

        vsg::dvec3 centre((cb.bounds.min + cb.bounds.max) * 0.5);

        auto delta = cb.bounds.max - cb.bounds.min;

        _wireframe->matrix = vsg::scale(delta) * vsg::translate(centre);
    }

    vsg::dmat4 SceneObject::transform(const vsg::dmat4& m) const
    {
        auto matrix = vsg::rotate(mult(_world_quat, _quat));
        matrix[3][0] = _position[0];
        matrix[3][1] = _position[1];
        matrix[3][2] = _position[2];

        return m * matrix;
    }

    SingleLoader::SingleLoader(vsg::ref_ptr<vsg::Node> loaded,
                               vsg::ref_ptr<vsg::Node> box,
                               const std::string &in_file,
                               const vsg::dvec3 &pos,
                               const vsg::dquat &in_quat,
                               const vsg::dmat4 &wtl)
        : vsg::Inherit<SceneObject, SingleLoader>(loaded, box, pos, in_quat, wtl)
        , file(in_file)
    {
    }
    SingleLoader::SingleLoader()
    {
    }
    SingleLoader::~SingleLoader() {}

    void SingleLoader::read(vsg::Input& input)
    {
        Node::read(input);

        input.read("quat", _quat);
        input.read("world_quat", _world_quat);
        input.read("filename", file);
        vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
        vsg::Path filename = vsg::findFile(file, searchPaths);
        addChild(vsg::read_cast<vsg::Node>(filename));

        input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        input.read("ltw", localToWorld);
        input.read("coord", _position);
    }

    void SingleLoader::write(vsg::Output& output) const
    {
        Node::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("filename", file);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("ltw", localToWorld);

        //vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

        output.write("coord", _position);
    }

    TerrainPoint::TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                               vsg::ref_ptr<vsg::BufferInfo> buffer,
                               const vsg::dmat4 &ltw,
                               vsg::ref_ptr<vsg::Node> compiled,
                               vsg::ref_ptr<vsg::Node> box,
                               vsg::stride_iterator<vsg::vec3> point)
        : vsg::Inherit<SceneObject, TerrainPoint>(compiled, box, ltw * vsg::dvec3(*point))
        , _worldToLocal(vsg::inverse(ltw))
        , _info(buffer)
        , _copyBufferCmd(copy)
        , _vertex(point)
    {
    }
    TerrainPoint::~TerrainPoint() {}

    void TerrainPoint::setPosition(const vsg::dvec3& position)
    {
        _position = position;
        *_vertex = _worldToLocal * position;
        _copyBufferCmd->copy(_info->data, _info);
    }

    RailPoint::RailPoint(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, const vsg::dvec3 &pos)
        : vsg::Inherit<SceneObject, RailPoint>(loaded, box, pos)
    {
        auto norm = vsg::normalize(pos);
        _world_quat = vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), norm);
        //always in world coordinates
        //_quat = quat;
    }
    RailPoint::RailPoint()
        : vsg::Inherit<SceneObject, RailPoint>()
    {
    }
    RailPoint::~RailPoint() {}

    void RailPoint::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("tangent", _tangent);
        input.read("tilt", _tilt);
        input.read("fstTraj", trajectory);
    }

    void RailPoint::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("tangent", _tangent);
        output.write("tilt", _tilt);
        output.write("fstTraj", trajectory);
    }

    void RailPoint::setPosition(const vsg::dvec3& position)
    {
        SceneObject::setPosition(position);
        recalculate();
    }
    void RailPoint::setRotation(const vsg::dquat &rotation)
    {
        SceneObject::setRotation(rotation);
        recalculate();
    }

    void RailPoint::recalculate()
    {
        if(trajectory) trajectory->recalculate();
    }

    vsg::dvec3 RailPoint::getTangent() const
    {
        return vsg::rotate(mult(_world_quat, _quat)) * vsg::dvec3(0.0, _tangent, 0.0);
    }

    vsg::dquat RailPoint::getTilt() const
    {
        return vsg::dquat(vsg::radians(_tilt), vsg::dvec3(0.0, 1.0, 0.0));
    }
/*
    void RailPoint::setInclination(double i)
    {
        auto angle = std::atan(i * 0.001);
        setRotation(mult(_quat, vsg::dquat(angle, vsg::dvec3(1.0, 0.0, 0.0))));
    }*/

    RailConnector::RailConnector(vsg::ref_ptr<vsg::Node> loaded,
                                 vsg::ref_ptr<vsg::Node> box,
                                 const vsg::dvec3 &pos)
        : QObject(nullptr)
        , vsg::Inherit<RailPoint, RailConnector>(loaded, box, pos) {}

    RailConnector::RailConnector()
        : vsg::Inherit<RailPoint, RailConnector>()
        , QObject(nullptr) {}

    RailConnector::~RailConnector() {}

    void RailConnector::read(vsg::Input &input)
    {
        RailPoint::read(input);

        input.read("sndTraj", fwdTrajectory);
        input.read("fwdSignal", fwdSignal);
        input.read("bwdSignal", bwdSignal);
    }

    void RailConnector::write(vsg::Output &output) const
    {
        RailPoint::write(output);

        output.write("sndTraj", fwdTrajectory);
        output.write("fwdSignal", fwdSignal);
        output.write("bwdSignal", bwdSignal);
    }

    void RailConnector::recalculate()
    {
        RailPoint::recalculate();
        if(fwdTrajectory) fwdTrajectory->recalculate();
    }

    std::pair<Trajectory*, bool> RailConnector::getFwd(const Trajectory *caller) const
    {
        if(!_reverser)
            return std::make_pair(fwdTrajectory, false);
        bool reversed = caller == fwdTrajectory;
        auto trj = reversed ? trajectory : fwdTrajectory;
        return std::make_pair(trj, reversed);
    }

    std::pair<Trajectory*, bool> RailConnector::getBwd(const Trajectory *caller) const
    {
        if(!_reverser)
            return std::make_pair(trajectory, false);
        bool reversed = caller == trajectory;
        auto trj = reversed ? fwdTrajectory : trajectory;
        return std::make_pair(trj, reversed);
    }

    void RailConnector::setFwd(Trajectory *caller)
    {
        if(fwdTrajectory == nullptr)
            fwdTrajectory = caller;
        else if(trajectory == nullptr)
        {
            _reverser = true;
            trajectory = caller;
        }
    }

    void RailConnector::setBwd(Trajectory *caller)
    {
        if(trajectory == nullptr)
            trajectory = caller;
        else if(fwdTrajectory == nullptr)
        {
            _reverser = true;
            fwdTrajectory = caller;
        }
    }
    void RailConnector::setFwdNull(Trajectory *caller)
    {
        if(caller == fwdTrajectory)
            fwdTrajectory = nullptr;
        else if(caller == trajectory)
        {
            trajectory = fwdTrajectory;
            fwdTrajectory = nullptr;
        }
        _reverser = false;
    }

    void RailConnector::setBwdNull(Trajectory *caller)
    {
        if(caller == trajectory)
            trajectory = nullptr;
        else if(caller == fwdTrajectory)
        {
            fwdTrajectory = trajectory;
            trajectory = nullptr;
        }
        _reverser = false;
    }

    bool RailConnector::isFree() const
    {
        return trajectory == nullptr || fwdTrajectory == nullptr;
    }

    void RailConnector::setSignal(vsg::ref_ptr<Signal> signal)
    {

    }

    void RailConnector::setReverseSignal(vsg::ref_ptr<Signal> signal)
    {

    }

    void RailConnector::receiveBwdDirState(State state)
    {
        if(!fwdSignal)
            emit sendBwdState(state);
        else
            fwdSignal->setFwdState(state);

    }

    void RailConnector::receiveFwdDirState(State state)
    {
        if(!bwdSignal)
            emit sendFwdState(state);
        else
            bwdSignal->setFwdState(state);
    }

    void RailConnector::receiveFwdDirRef()
    {
        if(!bwdSignal)
            emit sendFwdRef();
        else
            bwdSignal->Ref();
    }

    void RailConnector::receiveBwdDirRef()
    {
        if(!fwdSignal)
            emit sendBwdRef();
        else
            fwdSignal->Ref();
    }

    void RailConnector::receiveFwdDirUnref()
    {
        if(!bwdSignal)
            emit sendFwdUnref();
        else
            bwdSignal->Unref();
    }

    void RailConnector::receiveBwdDirUnref()
    {
        if(!fwdSignal)
            emit sendBwdUnref();
        else
            fwdSignal->Unref();
    }

    StaticConnector::StaticConnector(vsg::ref_ptr<vsg::Node> loaded,
                                     vsg::ref_ptr<vsg::Node> box,
                                     const vsg::dvec3 &pos)
        : vsg::Inherit<RailConnector, StaticConnector>(loaded, box, pos)
    {
        _world_quat = {0.0, 0.0, 0.0, 1.0};
    }

    StaticConnector::StaticConnector() : vsg::Inherit<RailConnector, StaticConnector>() {}

    StaticConnector::~StaticConnector() {}

    void StaticConnector::setPosition(const vsg::dvec3 &position) {}

    void StaticConnector::setRotation(const vsg::dquat &rotation) {}

    SwitchConnector::SwitchConnector(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box, const vsg::dvec3 &pos)
    {

    }

    SwitchConnector::SwitchConnector()
    {

    }

    SwitchConnector::~SwitchConnector()
    {

    }

    void SwitchConnector::setFwd(Trajectory *caller)
    {
        if(fwdTrajectory == nullptr)
            fwdTrajectory = caller;
        else if(sideTrajectory == nullptr)
            sideTrajectory = caller;
        else if(trajectory == nullptr)
        {
            _reverser = true;
            trajectory = caller;
        }
    }

    void SwitchConnector::switchState(bool state)
    {

    }

}
