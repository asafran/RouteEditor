#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/all.h>

class SceneObject : public vsg::Inherit<vsg::Transform, SceneObject>
{
public:
    explicit SceneObject(const vsg::dvec3 &pos = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});
    explicit SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});

    virtual ~SceneObject();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(const vsg::dmat4& m) const override;

    void setRotation(const vsg::dquat &q);

    vsg::dquat quat;
    vsg::dvec3 position;

protected:
    vsg::dquat world_quat;
};

class SingleLoader : public vsg::Inherit<SceneObject, SingleLoader>
{
public:
    SingleLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4& in_matrix = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});
    SingleLoader();

    virtual ~SingleLoader();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    std::string file;
};

template<typename T>
vsg::t_quat<T> mult(const vsg::t_quat<T>& lhs, const vsg::t_quat<T>& rhs)
{
    vsg::t_vec3<T> lhv(lhs.x, lhs.y, lhs.z);
    vsg::t_vec3<T> rhv(rhs.x, rhs.y, rhs.z);
    auto vec = vsg::cross(lhv, rhv) + (rhv * lhs[3]) + (lhv * rhs[3]);
    return vsg::t_quat<T>(vec.x, vec.y, vec.z, lhs[3] * rhs[3] - vsg::dot(lhv, rhv));
}

class Track : public vsg::Inherit<vsg::Object, Track>
{
public:
    explicit Track();

    virtual ~Track();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    virtual vsg::dvec3 position(double coord) const;

    double lenght;

    vsg::MatrixTransform ltw;

    vsg::MatrixTransform next;
};

class StraitTrack : public vsg::Inherit<Track, StraitTrack>
{
public:
    explicit StraitTrack();

    virtual ~StraitTrack();
/*
    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;
*/
    vsg::dvec3 position(double coord) const override;
};

class CurvedTrack : public vsg::Inherit<Track, CurvedTrack>
{
public:
    explicit CurvedTrack();

    virtual ~CurvedTrack();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dvec3 position(double coord) const override;

    double radius;
};
/*
struct Tracks
{
    vsg::ref_ptr<Track> track;

};
*/
class Trajectory : public vsg::Inherit<SceneObject, Trajectory>
{
public:
    explicit Trajectory(vsg::ref_ptr<vsg::Node> loaded, const vsg::dmat4& in_matrix, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});

    virtual ~Trajectory();

    void rebuildElevation();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    std::vector<vsg::ref_ptr<Track>> tracks;
    std::vector<std::string> names;
    std::vector<double> elevation;
    vsg::MatrixStack stack;
};

#endif // SCENEOBJECTS_H
