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
#include <vsg/io/read.h>

#include <QDir>
#include <QFileInfo>

#include "../tiny_obj_loader.h"

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

    struct VertexData
    {
        VertexData(const vsg::vec3 &vert)
        {
            verticle = vert;
        }

        vsg::vec3 verticle = {};

        vsg::vec2 uv = {};

        vsg::vec3 normal = {};
    };

    static inline std::pair<std::vector<VertexData>, vsg::ref_ptr<vsg::StateGroup> > loadData(std::string path, vsg::ref_ptr<const vsg::Options> options)
    {
        QFileInfo fi(path.c_str());

        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = fi.absolutePath().toStdString(); // Path to material files

        tinyobj::ObjReader reader;
        std::vector<VertexData> vd;

        if (!reader.ParseFromFile(path, reader_config))
        {
            std::make_pair(vd, nullptr);
        }

        auto& materials = reader.GetMaterials();

        auto texture = vsg::read_cast<vsg::Data>(fi.absolutePath().toStdString() + QDir::separator().toLatin1() + materials.front().diffuse_texname, options);

        auto state = createStateGroup(texture, options);

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();

        auto it = attrib.vertices.begin();
        auto end = attrib.vertices.end() - (attrib.vertices.size()/2);
        while (it < end)
        {
            vd.push_back(VertexData(vsg::vec3(*it, *(it + 2), *(it + 1))));
            it += 3;
        }

        for(const auto &index : shapes.front().mesh.indices)
        {
            if(index.vertex_index < vd.size())
            {
                auto& v = vd[index.vertex_index];
                v.uv = {0.0, attrib.texcoords.at((index.texcoord_index * 2) + 1)};
                auto idx = index.normal_index * 3;
                v.normal.x = attrib.normals.at(idx);
                v.normal.y = attrib.normals.at(idx + 2);
                v.normal.z = attrib.normals.at(idx + 1);
            }
        }
        return std::make_pair(vd, state);
    }
}

#endif
