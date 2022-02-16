#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/maths/transform.h>
#include <vsg/nodes/Transform.h>
#include <vsg/utils/Builder.h>
#include <vsg/commands/CopyAndReleaseBuffer.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/VertexInputState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorSet.h>
#include <vsg/io/Options.h>

class RailsPointEditor;
class PointsModel;

namespace route
{
    class Trajectory;
    class SplineTrajectory;

    class SceneObject : public vsg::Inherit<vsg::Transform, SceneObject>
    {
    public:
        SceneObject();
        SceneObject(vsg::ref_ptr<vsg::Node> box,
                    const vsg::dvec3 &pos,
                    const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0},
                    const vsg::dmat4 &ltw = {});
        SceneObject(vsg::ref_ptr<vsg::Node> loaded, vsg::ref_ptr<vsg::Node> box,
                    const vsg::dvec3 &pos = {},
                    const vsg::dquat &w_quat = {0.0, 0.0, 0.0, 1.0},
                    const vsg::dmat4 &ltw = {});


        virtual ~SceneObject();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        vsg::dmat4 transform(const vsg::dmat4& m) const override;

        //void setRotation(const vsg::dquat &q);
        virtual void setPosition(const vsg::dvec3& pos) { _position = pos; }
        virtual void setRotation(const vsg::dquat& rot) { _quat = rot; }

        void recalculateWireframe();
        void setSelection(bool selected) { _selected = selected; }

        void traverse(vsg::RecordTraversal& visitor) const override
        {
            Group::traverse(visitor);
            if(_selected)
                _wireframe->accept(visitor);
        }

        vsg::dvec3 getPosition() const { return _position; }
        vsg::dvec3 getWorldPosition() const { return localToWorld * _position; }
        vsg::dquat getWorldQuat() const { return _world_quat; }
        vsg::dquat getRotation() const { return _quat; }
        vsg::dquat getWorldRotation() const;

        bool isSelected() const { return _wireframe.valid(); }

        vsg::dmat4 localToWorld = {};

    protected:
        vsg::dvec3 _position = {};
        vsg::dquat _quat = {0.0, 0.0, 0.0, 1.0};

        vsg::ref_ptr<vsg::MatrixTransform> _wireframe = vsg::MatrixTransform::create();
        bool _selected;

        vsg::dquat _world_quat = {};
    };

    class SingleLoader : public vsg::Inherit<SceneObject, SingleLoader>
    {
    public:
        SingleLoader(vsg::ref_ptr<vsg::Node> loaded,
                     vsg::ref_ptr<vsg::Node> box,
                     const std::string &in_file,
                     const vsg::dvec3 &pos = {},
                     const vsg::dquat &in_quat = {0.0, 0.0, 0.0, 1.0},
                     const vsg::dmat4 &wtl = {});
        SingleLoader();

        virtual ~SingleLoader();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        std::string file;
    };

    enum Mask : uint64_t
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
                              vsg::ref_ptr<vsg::Node> box,
                              vsg::stride_iterator<vsg::vec3> point);

        virtual ~TerrainPoint();

        void setPosition(const vsg::dvec3& position) override;

    private:
        vsg::dmat4 _worldToLocal;

        vsg::ref_ptr<vsg::BufferInfo> _info;
        vsg::ref_ptr<vsg::CopyAndReleaseBuffer> _copyBufferCmd;
        vsg::stride_iterator<vsg::vec3> _vertex;
    };

    class RailPoint : public vsg::Inherit<SceneObject, RailPoint>
    {
    public:
        RailPoint(vsg::ref_ptr<vsg::Node> loaded,
                  vsg::ref_ptr<vsg::Node> box,
                  const vsg::dvec3 &pos);
        RailPoint();

        virtual ~RailPoint();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void setPosition(const vsg::dvec3& position) override;
        void setRotation(const vsg::dquat& rotation) override;

        virtual void recalculate();

        vsg::dvec3 getTangent() const;

        vsg::dquat getTilt() const;

        void setTangent(double t) { _tangent = t; recalculate(); }

        void setInclination(double i);

        Trajectory *trajectory = nullptr;

        void setTilt(double t) { _tilt = t; recalculate(); }

        void setCHeight(double h) { _cheight = h; recalculate(); }

    protected:
        double _tangent = 20.0;
        double _tilt = 0.0;
        double _cheight = 0.0;

        friend class ::RailsPointEditor;
        friend class ::PointsModel;
    };

    class RailConnector : public vsg::Inherit<RailPoint, RailConnector>
    {
    public:
        RailConnector(vsg::ref_ptr<vsg::Node> loaded,
                      vsg::ref_ptr<vsg::Node> box,
                      const vsg::dvec3 &pos);
        RailConnector();

        virtual ~RailConnector();

        void read(vsg::Input& input) override;
        void write(vsg::Output& output) const override;

        void recalculate() override;

        std::pair<Trajectory*, bool> getFwd(const Trajectory *caller) const;

        std::pair<Trajectory*, bool> getBwd(const Trajectory *caller) const;

        void setFwd(Trajectory *caller);

        void setBwd(Trajectory *caller);

        void setNull(Trajectory *caller);

        bool isFree() const;

        Trajectory *fwdTrajectory = nullptr;
    };

    class StaticConnector : public vsg::Inherit<RailConnector, StaticConnector>
    {
    public:
        StaticConnector(vsg::ref_ptr<vsg::Node> loaded,
                        vsg::ref_ptr<vsg::Node> box,
                        const vsg::dvec3 &pos);
        StaticConnector();

        virtual ~StaticConnector();

        void setPosition(const vsg::dvec3& position) override;
        void setRotation(const vsg::dquat& rotation) override;
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
    template<typename T>
    constexpr vsg::t_quat<T> toQuaternion(T x, T y, T z) // yaw (Z), pitch (Y), roll (X)
    {
        // Abbreviations for the various angular functions
        T cy = cos(z * 0.5);
        T sy = sin(z * 0.5);
        T cp = cos(y * 0.5);
        T sp = sin(y * 0.5);
        T cr = cos(x * 0.5);
        T sr = sin(x * 0.5);

        vsg::t_quat<T> q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;

        return q;
    }

    static inline vsg::ref_ptr<vsg::StateGroup> createStateGroup(vsg::ref_ptr<vsg::Data> textureData)
    {
        vsg::Paths searchPaths = vsg::getEnvPaths("RRS2_ROOT");

        vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", vsg::findFile("shaders/vert_PushConstants.spv", searchPaths));
        vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", vsg::findFile("shaders/frag_PushConstants.spv", searchPaths));
        if (!vertexShader || !fragmentShader)
        {
            //std::cout << "Could not create shaders." << std::endl;
            return vsg::ref_ptr<vsg::StateGroup>();
        }

        // set up graphics pipeline
        vsg::DescriptorSetLayoutBindings descriptorBindings{
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
        };

        auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

        vsg::PushConstantRanges pushConstantRanges{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection view, and model matrices, actual push constant calls automatically provided by the VSG's DispatchTraversal
        };

        vsg::VertexInputState::Bindings vertexBindingsDescriptions{
            VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
            VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // colour data
            VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}  // tex coord data
        };

        vsg::VertexInputState::Attributes vertexAttributeDescriptions{
            VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vertex data
            VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // colour data
            VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0}     // tex coord data
        };

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(),
            vsg::RasterizationState::create(),
            vsg::MultisampleState::create(),
            vsg::ColorBlendState::create(),
            vsg::DepthStencilState::create()};

        auto pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout}, pushConstantRanges);
        auto graphicsPipeline = vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
        auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

        // create texture image and associated DescriptorSets and binding
        auto descriptorTexture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{descriptorTexture});
        auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

        // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
        auto stateGroup = vsg::StateGroup::create();
        stateGroup->add(bindGraphicsPipeline);
        stateGroup->add(bindDescriptorSet);

        return stateGroup;
    }
}

#endif // SCENEOBJECTS_H
