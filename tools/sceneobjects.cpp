#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include <vsg/io/read.h>
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/MatrixTransform.h>
#include "topology.h"

namespace route
{
    SceneObject::SceneObject(const vsg::dvec3& pos, const vsg::dquat& w_quat, const vsg::dmat4 &wtl)
        : vsg::Inherit<vsg::Transform, SceneObject>()
        , _position(pos)
        , _quat(0.0, 0.0, 0.0, 1.0)
        , _world_quat(w_quat)
        , worldToLocal(wtl)
    {
    }

    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat& w_quat, const vsg::dmat4 &wtl)
        : SceneObject(pos, w_quat, wtl)
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
        input.read("wtl", worldToLocal);
        input.read("coord", _position);
    }

    void SceneObject::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("wtl", worldToLocal);

        output.write("coord", _position);
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
        input.read("wtl", worldToLocal);
        input.read("coord", _position);
    }

    void SingleLoader::write(vsg::Output& output) const
    {
        Node::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("filename", file);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("wtl", worldToLocal);

        //vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

        output.write("coord", _position);
    }

    TerrainPoint::TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                               vsg::ref_ptr<vsg::BufferInfo> buffer,
                               const vsg::dmat4 &ltw,
                               vsg::ref_ptr<vsg::Node> compiled,
                               vsg::stride_iterator<vsg::vec3> point)
        : vsg::Inherit<SceneObject, TerrainPoint>(compiled, ltw * vsg::dvec3(*point))
        , _info(buffer)
        , _copyBufferCmd(copy)
        , _vertex(point)
        , _wtl(vsg::inverse(ltw))
    {
    }
    TerrainPoint::~TerrainPoint() {}

    void TerrainPoint::setPosition(const vsg::dvec3& position)
    {
        _position = position;
        *_vertex = vsg::vec3(_wtl * position);
        _copyBufferCmd->copy(_info->data, _info);
    }

    SplinePoint::SplinePoint(const vsg::dvec3 &point, vsg::ref_ptr<vsg::Node> compiled)
        : vsg::Inherit<SceneObject, SplinePoint>(compiled, point)
    {
    }
    SplinePoint::SplinePoint()
        : vsg::Inherit<SceneObject, SplinePoint>()
    {
    }
    SplinePoint::~SplinePoint() {}


    void SplinePoint::setPosition(const vsg::dvec3& position)
    {
        SceneObject::setPosition(position);
        trajectory->recalculate();
    }
    void SplinePoint::setRotation(const vsg::dquat &rotation)
    {
        SceneObject::setRotation(rotation);
        trajectory->recalculate();
    }
}
