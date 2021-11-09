#include "sceneobjects.h"
#include <vsg/maths/quat.h>
#include <QDir>

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dmat4& in_matrix, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix)
    , quat(in_quat)
{
    addChild(loaded);
}
SceneObject::SceneObject()
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>()
    , quat(0.0, 0.0, 0.0, 1.0)
{
}

SceneObject::~SceneObject() {}

void SceneObject::read(vsg::Input& input)
{
    Group::read(input);

    vsg::dvec3 pos;

    input.read("quat", quat);
    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", pos);

    matrix = vsg::rotate(quat);

    matrix[3][0] = pos[0];
    matrix[3][1] = pos[1];
    matrix[3][2] = pos[2];
}

void SceneObject::write(vsg::Output& output) const
{
    Group::write(output);

    output.write("quat", quat);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    vsg::dvec3 pos = position();

    output.write("coord", pos);
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
    input.read("filename", file);
    vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
    vsg::Path filename = vsg::findFile(file, searchPaths);
    addChild(vsg::read_cast<vsg::Node>(filename));

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", pos);

    matrix = vsg::rotate(quat);

    matrix[3][0] = pos[0];
    matrix[3][1] = pos[1];
    matrix[3][2] = pos[2];
}

void SingleLoader::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("quat", quat);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

    output.write("coord", pos);
}
