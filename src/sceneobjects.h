#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/all.h>

class SceneObject : public vsg::Inherit<vsg::MatrixTransform, SceneObject>
{
public:
    explicit SceneObject(const vsg::dmat4& in_matrix = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});
    explicit SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dmat4& in_matrix = {}, const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0});

    virtual ~SceneObject();

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    vsg::dvec3 position() const { return vsg::dvec3(matrix[3][0], matrix[3][1], matrix[3][2]);; }

    void setRotation(const vsg::dquat &q);

    vsg::dquat quat;

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

#endif // SCENEOBJECTS_H
