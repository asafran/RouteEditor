#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include <vsg/io/read.h>
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/LOD.h>
#include "topology.h"

namespace route
{
    SceneObject::SceneObject(const vsg::dvec3& pos, const vsg::dquat& w_quat, const vsg::dmat4 &ltw)
        : vsg::Inherit<vsg::Transform, SceneObject>()
        , _position(pos)
        , _quat(0.0, 0.0, 0.0, 1.0)
        , _world_quat(w_quat)
        , localToWorld(ltw)
    {
    }

    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat& w_quat, const vsg::dmat4 &ltw)
        : SceneObject(pos, w_quat, ltw)
    {
        addChild(loaded);
    }

    SceneObject::SceneObject() : vsg::Inherit<vsg::Transform, SceneObject>() {}

    SceneObject::~SceneObject() {}

    void SceneObject::read(vsg::Input& input)
    {
        Group::read(input);

        input.read("quat", _quat);
        input.read("world_quat", _world_quat);
        input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        input.read("ltw", localToWorld);
        input.read("coord", _position);
    }

    void SceneObject::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("ltw", localToWorld);

        output.write("coord", _position);
    }
    vsg::dquat SceneObject::getWorldRotation() const
    {
        return mult(_world_quat, _quat);
    }

    void SceneObject::setWireframe(vsg::ref_ptr<vsg::Builder> builder)
    {
        vsg::ComputeBounds cb;
        t_traverse(*this, cb);

        vsg::vec3 centre((cb.bounds.min + cb.bounds.max) * 0.5);

        vsg::GeometryInfo info;
        vsg::StateInfo state;

        state.wireframe = true;
        state.lighting = false;

        auto delta = cb.bounds.max - cb.bounds.min;

        info.dx.set(delta.x, 0.0f, 0.0f);
        info.dy.set(0.0f, delta.y, 0.0f);
        info.dz.set(0.0f, 0.0f, delta.z);
        info.position = centre;
        info.transform = vsg::rotate(vsg::quat(_world_quat));

        _wireframe = builder->createBox(info, state);
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
                               const std::string &in_file,
                               const vsg::dvec3 &pos,
                               const vsg::dquat &in_quat,
                               const vsg::dmat4 &wtl)
        : vsg::Inherit<SceneObject, SingleLoader>(loaded, pos, in_quat, wtl)
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
                               vsg::stride_iterator<vsg::vec3> point)
        : vsg::Inherit<SceneObject, TerrainPoint>(compiled, ltw * vsg::dvec3(*point))
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

    RailPoint::RailPoint(vsg::ref_ptr<vsg::Node> compiled, const vsg::dvec3 &pos)
        : vsg::Inherit<SceneObject, RailPoint>(compiled, pos)
    {
        auto norm = vsg::normalize(pos);
        _world_quat = vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), norm);
        //always in world coordinates
    }
    RailPoint::RailPoint()
        : vsg::Inherit<SceneObject, RailPoint>()
    {
    }
    RailPoint::~RailPoint() {}

    void RailPoint::read(vsg::Input &input)
    {
        SceneObject::read(input);

        input.read("fstTraj", trajectory);
    }

    void RailPoint::write(vsg::Output &output) const
    {
        SceneObject::write(output);

        output.write("fstTraj", trajectory);
    }

    void RailPoint::setPosition(const vsg::dvec3& position)
    {
        SceneObject::setPosition(position);
        trajectory->recalculate();
    }
    void RailPoint::setRotation(const vsg::dquat &rotation)
    {
        SceneObject::setRotation(rotation);
        trajectory->recalculate();
    }

    vsg::dvec3 RailPoint::getTangent() const
    {
        auto q = mult(vsg::dquat(0.0, 0.0, 20.0, 0.0), mult(_world_quat, _quat));
        return vsg::dvec3(q.x, q.y, q.z);
    }

    RailConnector::RailConnector(vsg::ref_ptr<vsg::Node> compiled, const vsg::dvec3 &pos)
        : vsg::Inherit<RailPoint, RailConnector>(compiled, pos)
    {
    }
    RailConnector::RailConnector()
        : vsg::Inherit<RailPoint, RailConnector>()
    {
    }
    RailConnector::~RailConnector() {}

    void RailConnector::read(vsg::Input &input)
    {
        RailPoint::read(input);

        input.read("fwdTraj", fwdTrajectory);
    }

    void RailConnector::write(vsg::Output &output) const
    {
        RailPoint::write(output);

        output.write("fwdTraj", fwdTrajectory);
    }

    void RailConnector::setPosition(const vsg::dvec3& position)
    {
        RailPoint::setPosition(position);
        fwdTrajectory->recalculate();
    }
    void RailConnector::setRotation(const vsg::dquat &rotation)
    {
        RailPoint::setRotation(rotation);
        fwdTrajectory->recalculate();
    }

    std::pair<Trajectory*, bool> RailConnector::getFwd(const Trajectory *caller) const
    {
        bool reversed = caller == fwdTrajectory;
        auto trj = reversed ? trajectory : fwdTrajectory;
        return std::make_pair(trj, reversed);
    }

    std::pair<Trajectory*, bool> RailConnector::getBwd(const Trajectory *caller) const
    {
        bool reversed = caller == trajectory;
        auto trj = reversed ? fwdTrajectory : trajectory;
        return std::make_pair(trj, reversed);
    }

    void RailConnector::setFwd(Trajectory *caller)
    {
        if(fwdTrajectory != nullptr)
            trajectory = caller;
        else
            fwdTrajectory = caller;
    }

    void RailConnector::setBwd(Trajectory *caller)
    {
        if(trajectory != nullptr)
            fwdTrajectory = caller;
        else
            trajectory = caller;
    }
}
