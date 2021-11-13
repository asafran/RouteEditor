#include "sceneobjects.h"
#include <vsg/maths/quat.h>
#include <QDir>

SceneObject::SceneObject(const vsg::dmat4& in_matrix, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix)
    , quat(0.0, 0.0, 0.0, 1.0)
    , world_quat(in_quat)
{

}
SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dmat4& in_matrix, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix * vsg::rotate(in_quat))
    , quat(0.0, 0.0, 0.0, 1.0)
    , world_quat(in_quat)
{
    addChild(loaded);
}

SceneObject::~SceneObject() {}

void SceneObject::read(vsg::Input& input)
{
    Group::read(input);

    vsg::dvec3 pos;

    input.read("quat", quat);
    input.read("world_quat", world_quat);
    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", pos);

    matrix = vsg::translate(pos);
    setRotation(quat);
}

void SceneObject::write(vsg::Output& output) const
{
    Group::write(output);

    output.write("quat", quat);
    output.write("world_quat", world_quat);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    vsg::dvec3 pos = position();

    output.write("coord", pos);
}
 void SceneObject::setRotation(const vsg::dquat& q)
 {
     quat = q;
     auto newMat = vsg::rotate(mult(world_quat, quat));
     newMat[3][0] = matrix[3][0];
     newMat[3][1] = matrix[3][1];
     newMat[3][2] = matrix[3][2];
     matrix = newMat;
 }

SingleLoader::SingleLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4& in_matrix, const vsg::dquat &in_quat)
    : vsg::Inherit<SceneObject, SingleLoader>(loaded, in_matrix, in_quat)
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

    vsg::dvec3 pos;

    input.read("quat", quat);
    input.read("world_quat", world_quat);
    input.read("filename", file);
    vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
    vsg::Path filename = vsg::findFile(file, searchPaths);
    addChild(vsg::read_cast<vsg::Node>(filename));

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", pos);

    matrix = vsg::translate(pos);
    setRotation(quat);
}

void SingleLoader::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("quat", quat);
    output.write("world_quat", world_quat);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

    output.write("coord", pos);
}
