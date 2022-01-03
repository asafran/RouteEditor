#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include <vsg/io/read.h>
#include "topology.h"

namespace route
{
    SceneObject::SceneObject(const vsg::dvec3& pos, const vsg::dquat& w_quat)
        : vsg::Inherit<vsg::Transform, SceneObject>()
        , _position(pos)
        , _world_quat(w_quat)
    {
    }

    SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat& w_quat)
        : SceneObject(pos, w_quat)
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
        input.read("local", local);
        input.read("coord", _position);
    /*
        std::string name;
        input.read("trajName", name);
        trajectory = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);

        matrix = vsg::translate(pos);
        setRotation(quat);
    */
    }

    void SceneObject::write(vsg::Output& output) const
    {
        Group::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("local", local);

        //vsg::dvec3 pos = position();

        output.write("coord", _position);
    /*
        std::string name;
        trajectory->getValue(META_NAME, name);
        output.write("trajName", name);
        */
    }
    /*
    void SceneObject::setRotation(const vsg::dquat& q)
    {
        quat = q;
        auto newMat = vsg::rotate(mult(world_quat, quat));
        newMat[3][0] = matrix[3][0];
        newMat[3][1] = matrix[3][1];
        newMat[3][2] = matrix[3][2];
        matrix = newMat;
    }
    */
    vsg::dmat4 SceneObject::transform(const vsg::dmat4& m) const
    {
        auto matrix = vsg::rotate(mult(_world_quat, _quat));
        matrix[3][0] = _position[0];
        matrix[3][1] = _position[1];
        matrix[3][2] = _position[2];

        return m * matrix;
    }

    SingleLoader::SingleLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dvec3 &pos, const vsg::dquat &in_quat)
        : vsg::Inherit<SceneObject, SingleLoader>(loaded, pos, in_quat)
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
        input.read("local", local);
        input.read("coord", _position);
    }

    void SingleLoader::write(vsg::Output& output) const
    {
        Node::write(output);

        output.write("quat", _quat);
        output.write("world_quat", _world_quat);
        output.write("filename", file);
        output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
        output.write("local", local);

        //vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

        output.write("coord", _position);
    }

    Selection::Selection(std::vector<vsg::ref_ptr<SceneObject>> in_selected)
        : vsg::Inherit<SceneObject, Selection>(vsg::dvec3(0.0,0.0,0.0))
        , selected(in_selected)
    {
        setValue(META_NAME, "Группа объектов");
        local = true;
    }
    Selection::Selection()
        : vsg::Inherit<SceneObject, Selection>(vsg::dvec3(0.0,0.0,0.0))
    {
        setValue(META_NAME, "Группа объектов");
        local = true;
    }

    Selection::~Selection() {}

    void Selection::setPosition(const vsg::dvec3& position)
    {
        auto delta = position - _position;
        _position = position;
        for (auto object : selected)
        {
            auto newpos = object->getPosition() + delta;
            object->setPosition(newpos);
        }
    }

    TerrainPoint::TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                               vsg::ref_ptr<vsg::BufferInfo> buffer,
                               const vsg::dmat4 &wtl,
                               vsg::ref_ptr<vsg::Node> compiled,
                               vsg::stride_iterator<vsg::vec3> point)
        : vsg::Inherit<SceneObject, TerrainPoint>(compiled, wtl * vsg::dvec3(*point))
        , _info(buffer)
        , _copyBufferCmd(copy)
        , _vertex(point)
        , _wtl(wtl)
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
