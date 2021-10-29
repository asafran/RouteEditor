#include "sceneobjects.h"
#include <vsg/maths/quat.h>
#include <QDir>

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dmat4& in_localToWorld, const std::string &in_file, const vsg::dmat4& in_matrix, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix)
    , file(in_file)
    , localToWord(in_localToWorld)
    , quat(in_quat)
{
    addChild(loaded);
    setValue(META_NAME, file);
}
SceneObject::SceneObject()
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>()
    , file()
    , localToWord()
    , quat(0.0, 0.0, 0.0, 1.0)
{
    setValue(META_NAME, className());
}
//SceneObject::~SceneObject() {}

void SceneObject::read(vsg::Input& input)
{
    Node::read(input);

    vsg::dvec3 pos;

    input.read("quat", quat);
    input.read("filename", file);
    vsg::Paths searchPaths = vsg::getEnvPaths("VSG_FILE_PATH");
    vsg::Path filename = vsg::findFile("models/" + file, searchPaths);
    addChild(vsg::read_cast<vsg::Node>(filename));

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("ltw", localToWord);
    input.read("coord", pos);

    matrix = vsg::mat4_cast(quat);

    matrix[3][0] = pos[0];
    matrix[3][1] = pos[1];
    matrix[3][2] = pos[2];
}

void SceneObject::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("quat", quat);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    output.write("ltw", localToWord);

    vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

    output.write("coord", pos);
}

vsg::dmat4 SceneObject::world() const
{
    return matrix * localToWord;
}
