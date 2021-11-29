#include "sceneobjects.h"
#include "LambdaVisitor.h"
#include <vsg/maths/quat.h>
#include <QDir>

SceneObject::SceneObject(const vsg::dvec3& pos, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::Transform, SceneObject>()
    , quat(0.0, 0.0, 0.0, 1.0)
    , position(pos)
    , world_quat(in_quat)
{

}
SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat& in_quat)
    : vsg::Inherit<vsg::Transform, SceneObject>()
    , quat(0.0, 0.0, 0.0, 1.0)
    , position(pos)
    , world_quat(in_quat)
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

RailLoader::RailLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4 in_matrix)
    : vsg::Inherit<vsg::MatrixTransform, RailLoader>(in_matrix)
    , file(in_file)
{
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
    vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");
    vsg::Path filename = vsg::findFile(file, searchPaths);
    addChild(vsg::read_cast<vsg::Node>(filename));

    input.read("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}

void RailLoader::write(vsg::Output& output) const
{
    Node::write(output);

    output.write("matrix", matrix);
    output.write("filename", file);
    output.write("subgraphRequiresLocalFrustum", subgraphRequiresLocalFrustum);
}

Trajectory::Trajectory(const vsg::dvec3 &pos, const vsg::dquat &quat)
    : vsg::Inherit<SceneObject, Trajectory>(pos, quat)
    , matrixStack(1)
    , lenght(0.0)
{
    auto group = vsg::Group::create();
    group->setValue(META_NAME, "Пути");
    addChild(group);
}
Trajectory::Trajectory(vsg::ref_ptr<vsg::Node> node, const std::string &name, const vsg::dvec3 &pos, const vsg::dquat &quat)
    : Trajectory(pos, quat)
{
    addTrack(node, name);
}

Trajectory::~Trajectory() {}

void Trajectory::read(vsg::Input& input)
{
    SceneObject::read(input);

    input.read("matrixStack", matrixStack);

    input.read("lenght", lenght);

    /*
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
    */
}

void Trajectory::write(vsg::Output& output) const
{
    SceneObject::write(output);

    output.write("matrixStack", matrixStack);

    output.write("lenght", lenght);

    //output.write("files", files);
}

void Trajectory::addTrack(vsg::ref_ptr<vsg::Node> node, const std::string &name)
{
    auto track = node->getObject<Track>(META_TRACK);
    auto tracks = children.front().cast<vsg::Group>();
    auto loader = RailLoader::create(node, name, matrixStack.back());

    matrixStack.emplace_back(matrixStack.back() * track->transform(track->lenght));
    lenght += track->lenght;

    //track->ltw = vsg::inverse(vsg::translate(position)) * vsg::inverse(next);
    tracks->children.push_back(loader);
}

void Trajectory::removeTrack()
{
    auto track = children.front().cast<vsg::Group>()->children.back()->getObject<Track>(META_TRACK);
    matrixStack.pop_back();
    children.front().cast<vsg::Group>()->children.pop_back();
}

void Trajectory::recalculatePositions()
{
    auto group = children.front().cast<vsg::Group>()->children;

    auto matrix = matrixStack.begin();
    auto it = group.begin();

    //auto loader = it->cast<RailLoader>();
    //auto inclination = vsg::rotate(atan(loader->inclination / 1000), vsg::dvec3(1.0, 0.0, 0.0));

    //loader->matrix = inclination;
    //matrix++;
    //it++;

    //*matrix = *matrix * inclination;
    vsg::dmat4 next;
    for(; it != group.end(); ++it)
    {
        //auto track = loader->children.front()->getObject<Track>(META_TRACK); INCREMENT!!!!!
        auto loader = it->cast<RailLoader>();
        auto inclination = vsg::rotate(atan(loader->inclination / 1000), vsg::dvec3(1.0, 0.0, 0.0));

        loader->matrix = *matrix * next * inclination;
        matrix++;
        next = next * inclination * (*matrix * vsg::inverse(*(matrix - 1))) * vsg::inverse(inclination) * vsg::inverse(*matrix * vsg::inverse(*(matrix - 1)));
        //next[3] = 1.0;
     }
}

