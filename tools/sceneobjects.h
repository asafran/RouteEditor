#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
//#include "trajectory.h"
#include <vsg/nodes/Transform.h>
#include <vsg/commands/CopyAndReleaseBuffer.h>

namespace route
{
    class Trajectory;
    class SplineTrajectory;

    class SceneObject : public vsg::Inherit<vsg::Transform, SceneObject>
    {
    public:
        SceneObject();
        SceneObject(const vsg::dvec3 &pos, const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0});
        SceneObject(vsg::ref_ptr<vsg::Node> loaded, const vsg::dvec3 &pos = {}, const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0});


        virtual ~SceneObject();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        vsg::dmat4 transform(const vsg::dmat4& m) const override;

        //void setRotation(const vsg::dquat &q);
        virtual void setPosition(const vsg::dvec3& pos) { _position = pos; }
        virtual void setRotation(const vsg::dquat& rot) { _quat = rot; }

        vsg::dvec3 getPosition() const { return _position; }
        vsg::dquat getRotation() const { return _quat; }


        bool local;

    protected:
        vsg::dvec3 _position;
        vsg::dquat _quat;

        vsg::dquat _world_quat = {0.0, 0.0, 0.0, 1.0};
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

    class Selection : public vsg::Inherit<SceneObject, Selection>
    {
    public:
        Selection(std::vector<vsg::ref_ptr<SceneObject>> in_selected);
        Selection();

        virtual ~Selection();

        void setPosition(const vsg::dvec3& position) override;

        std::vector<vsg::ref_ptr<SceneObject>> selected;
    };

    enum Mask : uint32_t
    {
        Tiles = 0b1,
        SceneObjects = 0b10,
        Points = 0b100,
        Letters = 0b1000,
        Tracks = 0b10000
    };

    /*
    constexpr const std::type_info& getType(Mask type)
    {
        switch (type) {
        case Tiles:
            return typeid (vsg::Switch);
        case  SceneObjects:
            return typeid (SceneObject);
        case  Points:
            return typeid (SceneObject);
        case  Letters:
            return typeid (vsg::Switch);
        case Tracks:
            return typeid (SceneTrajectory);
        }
    }*/

    class TerrainPoint : public vsg::Inherit<SceneObject, TerrainPoint>
    {
    public:
        explicit TerrainPoint(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copy,
                              vsg::ref_ptr<vsg::BufferInfo> buffer,
                              const vsg::dmat4 &wtl,
                              vsg::ref_ptr<vsg::Node> compiled,
                              vsg::stride_iterator<vsg::vec3> point);

        virtual ~TerrainPoint();

        void setPosition(const vsg::dvec3& position) override;

    private:
        vsg::ref_ptr<vsg::BufferInfo> _info;
        vsg::ref_ptr<vsg::CopyAndReleaseBuffer> _copyBufferCmd;
        vsg::stride_iterator<vsg::vec3> _vertex;

        vsg::dmat4 _wtl;
    };

    class SplinePoint : public vsg::Inherit<SceneObject, SplinePoint>
    {
    public:
        SplinePoint(const vsg::dvec3 &point, vsg::ref_ptr<vsg::Node> compiled);
        SplinePoint();

        virtual ~SplinePoint();

        void setPosition(const vsg::dvec3& position) override;
        void setRotation(const vsg::dquat& rotation) override;

        SplineTrajectory *trajectory;
    };

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
}

#endif // SCENEOBJECTS_H
