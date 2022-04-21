#ifndef SCENETOOLS_H
#define SCENETOOLS_H

#include <vsg/nodes/StateGroup.h>
#include <vsg/io/Options.h>
#include <vsg/utils/SharedObjects.h>
#include <vsg/state/VertexInputState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorSet.h>
#include <vsg/utils/GraphicsPipelineConfig.h>
#include <vsg/state/ViewDependentState.h>
#include <vsg/state/material.h>

namespace route
{
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

    template<typename T>
    constexpr double toElevation(vsg::t_quat<T> q)
    {
        T sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
        T cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
        return sinr_cosp == 0 ? 0 : sinr_cosp / cosr_cosp * 1000;
    }

    template<typename T>
    constexpr bool isInEq(const vsg::t_quat<T>& lhs, const vsg::t_quat<T>& rhs)
    {
        return lhs[0] != rhs[0] || lhs[1] != rhs[1] || lhs[2] != rhs[2] || lhs[3] != rhs[3];
    }

    static inline vsg::ref_ptr<vsg::StateGroup> createStateGroup(vsg::ref_ptr<vsg::Data> textureData, vsg::ref_ptr<const vsg::Options> options)
    {
        /*
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
        */

        auto sharedObjects = options->sharedObjects;

        auto shaderSet = vsg::createPhongShaderSet(options);

        //shaderSet = createFlatShadedShaderSet(options);
        //shaderSet = createPhysicsBasedRenderingShaderSet(options);
        //shaderSet = createPhongShaderSet(options);

        auto graphicsPipelineConfig = vsg::GraphicsPipelineConfig::create(shaderSet);

        auto& defines = graphicsPipelineConfig->shaderHints->defines;

        //if (stateInfo.instance_positions_vec3) defines.push_back("VSG_INSTANCE_POSITIONS");

        // set up graphics pipeline
        vsg::Descriptors descriptors;


        // set up graphics pipeline
        vsg::DescriptorSetLayoutBindings descriptorBindings;

        auto sampler = vsg::Sampler::create();

        if (sharedObjects) sharedObjects->share(sampler);

        graphicsPipelineConfig->assignTexture(descriptors, "diffuseMap", textureData, sampler);

        // set up pass of material
        auto mat = vsg::PhongMaterialValue::create();
        mat->value().specular = vsg::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        graphicsPipelineConfig->assignUniform(descriptors, "material", mat);

        if (sharedObjects) sharedObjects->share(descriptors);


        // set up ViewDependentState
        defines.push_back("VSG_VIEW_LIGHT_DATA");
        vsg::ref_ptr<vsg::ViewDescriptorSetLayout> vdsl;
        if (sharedObjects) vdsl = sharedObjects->shared_default<vsg::ViewDescriptorSetLayout>();
        else vdsl = vsg::ViewDescriptorSetLayout::create();
        graphicsPipelineConfig->additionalDescrptorSetLayout = vdsl;


        graphicsPipelineConfig->enableArray("vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, 12);
        graphicsPipelineConfig->enableArray("vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, 12);
        graphicsPipelineConfig->enableArray("vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, 8);

        graphicsPipelineConfig->enableArray("vsg_Color", VK_VERTEX_INPUT_RATE_INSTANCE, 16);


        graphicsPipelineConfig->rasterizationState->cullMode = VK_CULL_MODE_BACK_BIT;

        graphicsPipelineConfig->colorBlendState->attachments = vsg::ColorBlendState::ColorBlendAttachments{
            {true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_SUBTRACT, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

        // if required initialize GraphicsPipeline/Layout etc.
        if (sharedObjects) sharedObjects->share(graphicsPipelineConfig, [](auto gpc) { gpc->init(); });
        else graphicsPipelineConfig->init();

        auto descriptorSet = vsg::DescriptorSet::create(graphicsPipelineConfig->descriptorSetLayout, descriptors);
        if (sharedObjects) sharedObjects->share(descriptorSet);

        auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineConfig->layout, 0, descriptorSet);
        if (sharedObjects) sharedObjects->share(bindDescriptorSet);

        // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
        auto stateGroup = vsg::StateGroup::create();
        stateGroup->add(graphicsPipelineConfig->bindGraphicsPipeline);
        stateGroup->add(bindDescriptorSet);

        auto bindViewDescriptorSets = vsg::BindViewDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineConfig->layout, 1);
        if (sharedObjects) sharedObjects->share(bindViewDescriptorSets);
        stateGroup->add(bindViewDescriptorSets);

        //if (sharedObjects) sharedObjects->report(std::cout);

        return stateGroup;
    } 
}

#endif
