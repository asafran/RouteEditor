#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include "trackobjects.h"
//#include "trajectory.h"
#include "LambdaVisitor.h"
#include <vsg/all.h>

class Trajectory;

class SceneObject : public vsg::Inherit<vsg::Transform, SceneObject>
{
public:
    SceneObject();
    SceneObject(const vsg::dvec3 &pos, const vsg::dquat &w_quat);
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos, const vsg::dquat &w_quat);
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, Trajectory *traj, double coord);
    SceneObject(Trajectory *traj, double coord);


    virtual ~SceneObject();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(const vsg::dmat4& m) const override;

    //void setRotation(const vsg::dquat &q);

    vsg::dquat quat = {0.0, 0.0, 0.0, 1.0};
    vsg::dvec3 position = {};

    Trajectory *trajectory;
    double trajCoord = 0.0;

protected:
    vsg::dquat world_quat = {0.0, 0.0, 0.0, 1.0};
};

class SingleLoader : public vsg::Inherit<SceneObject, SingleLoader>
{
public:
    SingleLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dvec3 &pos = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});
    SingleLoader();

    virtual ~SingleLoader();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    std::string file;
};
/*
class RailLoader : public vsg::Inherit<vsg::Transform, RailLoader>, public TrackSection
{
public:
    RailLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file, const vsg::dmat4 in_matrix);
    RailLoader();

    virtual ~RailLoader();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dmat4 transform(const vsg::dmat4& m) const override { return m * matrix; }

    std::string file;
};
/*
class SimpleSingleLoader : public vsg::Inherit<vsg::Node, SimpleSingleLoader>
{
public:
    SimpleSingleLoader(vsg::ref_ptr<vsg::Node> loaded, const std::string &in_file);
    SimpleSingleLoader();

    virtual ~SimpleSingleLoader();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    std::string file;
};

class Junction : public vsg::Inherit<SceneObject, Junction>, public Trajectory
{
public:
    explicit Junction(vsg::ref_ptr<vsg::Node> node, const std::string &name, const vsg::dvec3 &pos = {}, const vsg::dquat &quat = {0.0, 0.0, 0.0, 1.0});
    explicit Junction(const vsg::dvec3 &pos = {}, const vsg::dquat &quat = {0.0, 0.0, 0.0, 1.0});

    virtual ~Junction();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;


protected:

};
*/
template<typename T>
vsg::t_quat<T> mult(const vsg::t_quat<T>& lhs, const vsg::t_quat<T>& rhs)
{
    vsg::t_vec3<T> lhv(lhs.x, lhs.y, lhs.z);
    vsg::t_vec3<T> rhv(rhs.x, rhs.y, rhs.z);
    auto vec = vsg::cross(lhv, rhv) + (rhv * lhs[3]) + (lhv * rhs[3]);
    return vsg::t_quat<T>(vec.x, vec.y, vec.z, lhs[3] * rhs[3] - vsg::dot(lhv, rhv));
}

template<typename T>
vsg::t_vec3<T> mult(const vsg::t_quat<T>& lhs, const vsg::t_vec3<T>& rhs)
{
    vsg::t_quat<T> rhq(rhs.x, rhs.y, rhs.z, 0.0);
    auto quat = mult(rhq, lhs);
    return vsg::t_vec3<T>(quat.x, quat.y, quat.z);
}

template<typename T>
constexpr vsg::t_quat<T> inv(const vsg::t_quat<T>& v)
{
    vsg::t_quat<T> c = conjugate(v);
    T inverse_len = static_cast<T>(1.0) / length(v);
    return vsg::t_quat<T>(c[0] * inverse_len, c[1] * inverse_len, c[2] * inverse_len, c[3] * inverse_len);
}

inline vsg::ref_ptr<vsg::Group> lowTile(const vsg::LineSegmentIntersector::Intersection &intersection, uint64_t frameCount)
{
    /*
    auto find = std::find_if(intersection.nodePath.crbegin(), intersection.nodePath.crend(), isCompatible<vsg::PagedLOD>);
    if(find != intersection.nodePath.crend())
    {*/
    if(intersection.nodePath.size() > 6)
    {
        auto plod = (*(intersection.nodePath.crbegin() + 6))->cast<vsg::PagedLOD>();
        if(plod)
            if(plod->highResActive(frameCount))
                if(plod->children.front().node.cast<vsg::Group>()->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                    return plod->children.front().node.cast<vsg::Group>();
    }
    return vsg::ref_ptr<vsg::Group>();
}



#endif // SCENEOBJECTS_H
