#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>
#include "topology.h"

SceneObject::SceneObject()
    : vsg::Inherit<vsg::Transform, SceneObject>()
{
}

SceneObject::SceneObject(const vsg::dvec3& pos, const vsg::dquat& w_quat)
    : vsg::Inherit<vsg::Transform, SceneObject>()
    , position(pos)
    , world_quat(w_quat)
{
}

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat& w_quat)
    : SceneObject(pos, w_quat)
{
    addChild(loaded);
}

SceneObject::SceneObject(Trajectory *traj, double coord)
    : vsg::Inherit<vsg::Transform, SceneObject>()
    , trajectory(traj)
    , trajCoord(coord)
{
}

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, Trajectory *traj, double coord)
    : SceneObject(traj, coord)
{
    addChild(loaded);
}


SceneObject::~SceneObject() {}

void SceneObject::read(vsg::Input& input)
{
    Group::read(input);

    input.read("quat", quat);
    input.read("world_quat", world_quat);
    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", position);

    std::string name;
    input.read("trajName", name);
    trajectory = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);
/*
    matrix = vsg::translate(pos);
    setRotation(quat);
*/
}

void SceneObject::write(vsg::Output& output) const
{
    Group::write(output);

    output.write("quat", quat);
    output.write("world_quat", world_quat);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    //vsg::dvec3 pos = position();

    output.write("coord", position);

    std::string name;
    trajectory->getValue(META_NAME, name);
    output.write("trajName", name);
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
    auto matrix = vsg::rotate(mult(world_quat, quat));
    matrix[3][0] = position[0];
    matrix[3][1] = position[1];
    matrix[3][2] = position[2];
    if(trajectory != nullptr)
        return m * trajectory->transform(trajectory->getPosition(trajectory->getLength())) * matrix;
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

    input.read("quat", quat);
    input.read("world_quat", world_quat);
    input.read("filename", file);
    vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
    vsg::Path filename = vsg::findFile(file, searchPaths);
    addChild(vsg::read_cast<vsg::Node>(filename));

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("coord", position);
}

void SingleLoader::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("quat", quat);
    output.write("world_quat", world_quat);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);

    //vsg::dvec3 pos(matrix[3][0], matrix[3][1], matrix[3][2]);

    output.write("coord", position);
}
/*
RailLoader::RailLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4 in_matrix)
    : vsg::Inherit<vsg::Transform, RailLoader>()
    , TrackSection()
    , file(in_file)
{
    track = loaded->getObject<Track>(META_TRACK);
    matrix = in_matrix;
    addChild(loaded);
}
RailLoader::RailLoader()
{
}
RailLoader::~RailLoader() {}

void RailLoader::read(vsg::Input& input)
{
    Node::read(input);

    input.read("matrix", matrix);
    input.read("filename", file);
    vsg::Path filename = vsg::findFile(file, input.options);
    auto node = vsg::read_cast<vsg::Node>(filename, input.options);
    addChild(node);
    track = node->getObject<Track>(META_TRACK);

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    input.read("incl", inclination);
}

void RailLoader::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("matrix", matrix);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
    output.write("incl", inclination);
}

SceneTrajectory::SceneTrajectory(const vsg::dvec3 &pos, const vsg::dquat &quat)
    : vsg::Inherit<SceneObject, SceneTrajectory>(pos, quat)
{
    auto group = vsg::Group::create();
    group->setValue(META_NAME, "Пути");
    addChild(group);
}
SceneTrajectory::SceneTrajectory(Trajectory *trajectory, const vsg::dvec3 &pos, const vsg::dquat &quat)
    : SceneTrajectory(pos, quat)
{
    traj = trajectory;
}

SceneTrajectory::~SceneTrajectory() {}

void SceneTrajectory::read(vsg::Input& input)
{
    Node::read(input);

    std::string name;
    input.read("trajName", name);

    traj = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);


    //input.read("files", files);

    vsg::dmat4 matrix;
    for(auto &file : files)
    {
        vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
        vsg::Path filename = vsg::findFile(file, searchPaths);
        auto tracknode = vsg::read_cast<vsg::Node>(filename);
        auto track = tracknode->getObject<Track>("Trk");
        tracks.emplace_back(track);
        auto transform = vsg::MatrixTransform::create(matrix);
        transform->addChild(tracknode);
        addChild(transform);
        track->ltw = vsg::inverse(vsg::translate(position)) * vsg::inverse(matrix);
        matrix = vsg::translate(track->position(track->lenght)) * matrix;
    }

}

void SceneTrajectory::write(vsg::Output& output) const
{
    Node::write(output);

    std::string name;
    traj->getValue(META_NAME, name);
    output.write("trajName", name);

    //output.write("files", files);
}

Junction::Junction(const vsg::dvec3 &pos, const vsg::dquat &quat)
    : vsg::Inherit<SceneObject, Junction>(pos, quat)
    , matrixStack(1)
    , lenght(0.0)
{
    auto group = vsg::Group::create();
    group->setValue(META_NAME, "Пути");
    addChild(group);
}
Junction::Junction(vsg::ref_ptr<vsg::Node> node, const std::string &name, const vsg::dvec3 &pos, const vsg::dquat &quat)
    : Junction(pos, quat)
{
    addTrack(node, name);
}

Junction::~Junction() {}

void Junction::read(vsg::Input& input)
{
    SceneObject::read(input);


}

void Junction::write(vsg::Output& output) const
{

}
*/

