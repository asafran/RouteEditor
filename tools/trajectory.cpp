#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <execution>
#include    <vsg/io/read.h>
#include    "topology.h"
#include    "sceneobjects.h"
#include    <vsg/all.h>


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace route
{
    SplineTrajectory::SplineTrajectory(std::string name,
                                       vsg::ref_ptr<RailConnector> bwdPoint,
                                       vsg::ref_ptr<RailConnector> fwdPoint,
                                       vsg::ref_ptr<vsg::Builder> builder,
                                       //vsg::ref_ptr<Compiler> compiler,
                                       tinyobj::attrib_t rail, vsg::ref_ptr<vsg::Data> texture,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name)
      , _builder(builder)
      //, _compiler(compiler)
      , _texture(texture)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
      , _fwdPoint(fwdPoint)
      , _bwdPoint(bwdPoint)
    {
        auto it = rail.vertices.begin();
        auto end = rail.vertices.end() - (rail.vertices.size()/2);
        while (it < end)
        {
            _geometry.push_back(vsg::vec3(*it, *(it + 1), *(it + 2)));
            it += 3;
        }

        _points.push_back(fwdPoint);
        _points.push_back(bwdPoint);

        fwdPoint->setBwd(this);
        bwdPoint->setFwd(this);

        it = rail.texcoords.begin();
        end = rail.texcoords.end() - (rail.texcoords.size()/2);
        while (it < end)
            _uv1.push_back(vsg::vec2(*it++, *it++));

        end = rail.texcoords.end();
        while (it < end)
            _uv2.push_back(vsg::vec2(*it++, *it++));

        recalculate();
    }

    SplineTrajectory::SplineTrajectory()
      : vsg::Inherit<Trajectory, SplineTrajectory>()
    {
    }
    SplineTrajectory::~SplineTrajectory()
    {
        _bwdPoint->setFwd(nullptr);
        _fwdPoint->setBwd(nullptr);
    }
    //------------------------------------------------------------------------------
    //
    //------------------------------------------------------------------------------

    void SplineTrajectory::read(vsg::Input& input)
    {
        /*
        Object::read(input);

        input.read("sections", sections);
        input.read("lenght", lenght);
        input.read("frontReversed", frontReversed);
        input.read("matrixStack", matrixStack);

        input.read("fwd", fwdTraj);
        */
    }

    void SplineTrajectory::write(vsg::Output& output) const
    {
        /*
        Object::write(output);

        output.write("sections", sections);
        output.write("lenght", lenght);
        output.write("frontReversed", frontReversed);
        output.write("matrixStack", matrixStack);

        output.write("fwd", fwdTraj);
        */
    }

    vsg::dvec3 SplineTrajectory::getPosition(double x) const
    {
        double T = ArcLength::solveLength(*_railSpline, 0.0, x);
        return _railSpline->getPosition(T);
    }

    double SplineTrajectory::invert(const vsg::dvec3 vec) const
    {
        SplineInverter<vsg::dvec3, double> inverter(*_railSpline);
        auto T = inverter.findClosestT(vec);
        return _railSpline->arcLength(0, T);
    }

    vsg::dmat4 SplineTrajectory::getMatrixAt(double x) const
    {
        double T = ArcLength::solveLength(*_railSpline, 0.0, x);
        auto pt = _railSpline->getTangent(T);
        return InterpolatedPTM(std::move(pt)).calculated;
    }

    void SplineTrajectory::recalculate()
    {

        std::vector<vsg::dvec3> points;
        std::vector<vsg::dvec3> tangents;

        auto front = _points.front()->getPosition();

        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(points),
                       [](const vsg::ref_ptr<RailPoint> sp)
        {
            return sp->getPosition();
        });
        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(tangents),
                       [](const vsg::ref_ptr<RailPoint> sp)
        {
            return sp->getTangent();
        });

        //points.push_back(_fwdPoint->getPosition() - front);
        //tangents.push_back(_fwdPoint->getTangent());

        _railSpline.reset(new InterpolationSpline(points, tangents));

        auto partitionBoundaries = ArcLength::partition(*_railSpline, _sleepersDistance);

        std::vector<InterpolatedPTM> derivatives(partitionBoundaries.size());
        std::transform(std::execution::par_unseq, partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [railSpline=_railSpline, front](const double T)
        {
            return std::move(InterpolatedPTM(railSpline->getTangent(T), front));
        });

        //std::mutex m;
        _track = vsg::MatrixTransform::create(vsg::translate(front));
        std::for_each(derivatives.begin(), derivatives.end(), [ sleeper=_sleeper, group=_track](const InterpolatedPTM &ptcm)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(sleeper);
            //std::lock_guard<std::mutex> guard(m);
            group->addChild(transform);
        });

        //auto last = std::unique(std::execution::par, derivatives.begin(), derivatives.end()); //vectorization is not supported
        //derivatives.erase(last, derivatives.end());
/*
        auto vertices = std::transform_reduce(derivatives.begin(), derivatives.end(), std::vector<vsg::vec3>(),
        [](auto vertices, auto transformed)
        {
            if(vertices.empty())
                return transformed;
            vertices.insert(std::end(vertices), std::begin(transformed), std::end(transformed));
            return vertices;
        },
        [geometry=_geometry](const InterpolatedPTCM &ptcm)
        {
            vsg::vec3 offset(1.0, 0.0, 0.0);

            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            auto copy = geometry;

            for(auto vec = copy.begin(); vec != copy.end(); ++vec)
            {
                *vec = fmat * (*vec + offset);
            }

            return copy;
        });*/

        std::vector<std::vector<vsg::vec3>> vertexGroups(derivatives.size());

        //auto norm = vsg::normalize(front);
        //auto w_mat = vsg::rotate(vsg::quat(vsg::vec3(0.0, 0.0, 1.0), vsg::vec3(norm)));

        std::transform(std::execution::par_unseq, derivatives.begin(), derivatives.end(), vertexGroups.begin(),
        [geometry=_geometry](const InterpolatedPTM &ptcm)
        {
            vsg::vec3 offset(0.0, 0.0, 0.0);

            auto fmat = static_cast<vsg::mat4>(ptcm.calculated);
            auto copy = geometry;

            for(auto vec = copy.begin(); vec != copy.end(); ++vec)
            {
                *vec = fmat * (*vec + offset);
            }

            return std::move(copy);
        });

        auto vsize = vertexGroups.size() * _geometry.size();
        auto vertArray = vsg::vec3Array::create(vsize);
        auto texArray = vsg::vec2Array::create(vsize);
        auto vertIt = vertArray->begin();
        auto texIt = texArray->begin();

        bool flip = false;

        for(const auto &vertexGroup : vertexGroups)
        {
            auto uv = flip ? _uv2.begin() : _uv1.begin();
            auto end = flip ? _uv2.end() : _uv1.end();
            for(const auto &vertex : vertexGroup)
            {
                Q_ASSERT(vertIt != vertArray->end());
                Q_ASSERT(uv < end);
                *vertIt = vertex;
                *texIt = *uv;
                texIt++;
                uv++;
                vertIt++;
            }
            flip = !flip;
        }

        std::vector<uint16_t> indices;
        const auto next = static_cast<uint16_t>(_geometry.size());
        uint16_t max = vsize - next - 1;

        for (uint16_t ind = 0; ind < max; ++ind)
        {
            if((ind + 1) % next == 0)
                continue;
            indices.push_back(ind);
            indices.push_back(ind + next);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + 1);
            indices.push_back(ind);
        }

        auto colorArray = vsg::vec4Array::create(vsize);
        std::fill(colorArray->begin(), colorArray->end(), vsg::vec4(1.0f, 0.0f, 1.0f, 1.0f));

        auto normalArray = vsg::vec3Array::create(vsize);
        std::fill(normalArray->begin(), normalArray->end(), vsg::vec3(1.0f, 0.0f, 0.0f));

        auto ind = vsg::ushortArray::create(indices.size());
        std::copy(indices.begin(), indices.end(), ind->begin());

        assignRails(vsg::DataList{vertArray, normalArray, texArray}, ind);
    }

    void SplineTrajectory::assignRails(vsg::DataList list, vsg::ref_ptr<vsg::ushortArray> indices)
    {
/*
        auto stateGroup = vsg::StateGroup::create();

        // load shaders
        auto vertexShader = vsg::read_cast<vsg::ShaderStage>("shaders/assimp.vert", _builder->options);
        //if (!vertexShader) vertexShader = assimp_vert(); // fallback to shaders/assimp_vert.cppp

        auto fragmentShader = vsg::read_cast<vsg::ShaderStage>("shaders/assimp_phong.frag", _builder->options);
        //if (!fragmentShader) fragmentShader = assimp_phong_frag();


        if (!vertexShader || !fragmentShader)
        {
            std::cout << "Could not create shaders." << std::endl;
            return;
        }

        auto shaderHints = vsg::ShaderCompileSettings::create();
        std::vector<std::string>& defines = shaderHints->defines;

        vertexShader->module->hints = shaderHints;
        vertexShader->module->code = {};

        fragmentShader->module->hints = shaderHints;
        fragmentShader->module->code = {};

        // set up graphics pipeline
        vsg::DescriptorSetLayoutBindings descriptorBindings;
        if (image)
        {
            // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
            descriptorBindings.push_back(VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
            defines.push_back("VSG_DIFFUSE_MAP");
        }

        if (stateInfo.displacementMap)
        {
            // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
            descriptorBindings.push_back(VkDescriptorSetLayoutBinding{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
            defines.push_back("VSG_DISPLACEMENT_MAP");
        }

        {
            descriptorBindings.push_back(VkDescriptorSetLayoutBinding{10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        }

        auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

        vsg::DescriptorSetLayouts descriptorSetLayouts{descriptorSetLayout};

        vsg::PushConstantRanges pushConstantRanges{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection view, and model matrices, actual push constant calls automatically provided by the VSG's DispatchTraversal
        };

        auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    //#if FLOAT_COLORS
        uint32_t colorSize = sizeof(vsg::vec4);
        VkFormat colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    #else
        uint32_t colorSize = sizeof(ubvec4);
        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    #endif

        vsg::VertexInputState::Bindings vertexBindingsDescriptions{
            VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
            VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // normal data
            VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}  // tex coord data
        };

        vsg::VertexInputState::Attributes vertexAttributeDescriptions{
            VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vertex data
            VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // normal data
            VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0}     // tex coord data
        };

        if (stateInfo.instancce_colors_vec4)
        {
            uint32_t colorBinding = static_cast<uint32_t>(vertexBindingsDescriptions.size());
            vertexBindingsDescriptions.push_back(VkVertexInputBindingDescription{colorBinding, colorSize, VK_VERTEX_INPUT_RATE_INSTANCE}); // color data
            vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{3, colorBinding, colorFormat, 0});                     // color data

        auto rasterState = vsg::RasterizationState::create();
        rasterState->cullMode = VK_CULL_MODE_BACK_BIT;

        auto colorBlendState = vsg::ColorBlendState::create();
        colorBlendState->attachments = vsg::ColorBlendState::ColorBlendAttachments{
            {true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_SUBTRACT, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

        auto inputAssemblyState = vsg::InputAssemblyState::create();

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            inputAssemblyState,
            rasterState,
            vsg::MultisampleState::create(),
            colorBlendState,
            vsg::DepthStencilState::create()};

        auto graphicsPipeline = vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);

        auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

        stateGroup->add(bindGraphicsPipeline);

        //auto displacementMap = stateInfo.displacementMap;

        // create texture image and associated DescriptorSets and binding
        auto mat = vsg::PhongMaterialValue::create();
        auto material = vsg::DescriptorBuffer::create(mat, 10);

        vsg::Descriptors descriptors;
        if (image)
        {
            auto sampler = vsg::Sampler::create();
            sampler->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            auto texture = vsg::DescriptorImage::create(sampler, image, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            descriptors.push_back(texture);
        }

        if (displacementMap)
        {
            auto sampler = Sampler::create();
            sampler->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            auto texture = DescriptorImage::create(sampler, displacementMap, 6, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            descriptors.push_back(texture);
        }

        descriptors.push_back(material);

        auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, descriptors);
        auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, vsg::DescriptorSets{descriptorSet});

        stateGroup->add(bindDescriptorSets);
        */

        auto vid = vsg::VertexIndexDraw::create();

        vid->assignArrays(list);

        vid->assignIndices(indices);
        vid->indexCount = static_cast<uint32_t>(indices->size());
        vid->instanceCount = 1;

        vsg::StateInfo si;
        si.image = _texture;
        si.lighting = false;

        auto stateGroup = _builder->createStateGroup(si);

        stateGroup->addChild(vid);

        _builder->compile(stateGroup);

        _track->addChild(stateGroup);

        //_builder->compile(_track);
    }

    void SplineTrajectory::add(vsg::ref_ptr<RailPoint> rp)
    {
        SplineInverter<vsg::dvec3, double> inverter(*_railSpline);
        double t = inverter.findClosestT(rp->getPosition());
        auto index = static_cast<size_t>(std::floor(t)) + 1;
        auto it = _points.begin() + index;
        Q_ASSERT(it <= _points.end());
        _points.insert(it, rp);
        rp->trajectory = this;

        recalculate();
    }

    SceneTrajectory::SceneTrajectory()
        : vsg::Inherit<vsg::Group, SceneTrajectory>()
    {
    }
    SceneTrajectory::SceneTrajectory(Trajectory *traj)
        : SceneTrajectory()
    {
        children.emplace_back(traj);
    }

    SceneTrajectory::~SceneTrajectory() {}

    void SceneTrajectory::read(vsg::Input& input)
    {
        Node::read(input);

        std::string name;
        input.read("trajName", name);

        addChild(input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name));

    }

    void SceneTrajectory::write(vsg::Output& output) const
    {
        Node::write(output);

        auto trajectory = children.front();
        Q_ASSERT(trajectory);

        std::string name;
        trajectory->getValue(META_NAME, name);
        output.write("trajName", name);

    }
}
