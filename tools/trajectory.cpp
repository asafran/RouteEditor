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
    SplineTrajectory::SplineTrajectory(std::string name, vsg::ref_ptr<vsg::Builder> builder,
                                       std::vector<vsg::ref_ptr<SplinePoint>> points,
                                       std::vector<vsg::vec3> geometry,
                                       vsg::ref_ptr<vsg::Node> sleeper, double distance)
      : vsg::Inherit<Trajectory, SplineTrajectory>(name)
      , _points(points)
      , _builder(builder)
      , _geometry(geometry)
      , _sleeper(sleeper)
      , _sleepersDistance(distance)
    {
        recalculate();
    }

    SplineTrajectory::SplineTrajectory()
      : vsg::Inherit<Trajectory, SplineTrajectory>()
    {
    }
    SplineTrajectory::~SplineTrajectory()
    {
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
        return InterpolatedPTCM(_railSpline->getCurvature(T)).calculated;
    }

    void SplineTrajectory::recalculate()
    {
        std::vector<vsg::dvec3> points;
        auto front = _points.front()->getPosition();
        std::transform(_points.begin(), _points.end(), std::back_insert_iterator(points),
                       [front](const vsg::ref_ptr<SplinePoint> sp)
        {
            return sp->getPosition() - front;
        });

        _railSpline.reset(new UniformCRSpline<vsg::dvec3, double>(points));

        auto partitionBoundaries = ArcLength::partition(*_railSpline, _sleepersDistance);

        std::vector<InterpolatedPTCM> derivatives(partitionBoundaries.size());
        std::transform(std::execution::par_unseq, partitionBoundaries.begin(), partitionBoundaries.end(), derivatives.begin(),
                       [railSpline=_railSpline](const double T)
        {
            return std::move(InterpolatedPTCM(railSpline->getCurvature(T)));
        });

        //std::mutex m;
        auto group = vsg::MatrixTransform::create(vsg::translate(front));
        std::for_each(derivatives.begin(), derivatives.end(), [ sleeper=_sleeper, group](const InterpolatedPTCM &ptcm)
        {
            auto transform = vsg::MatrixTransform::create(ptcm.calculated);
            transform->addChild(sleeper);
            //std::lock_guard<std::mutex> guard(m);
            group->addChild(transform);
        });

        //auto last = std::unique(std::execution::par, derivatives.begin(), derivatives.end()); //vectorization is not supported
        //derivatives.erase(last, derivatives.end());

        auto vertices = std::transform_reduce(std::execution::par, derivatives.begin(), derivatives.end(), std::vector<vsg::vec3>(),
        [](auto vertices, auto transformed)
        {
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
        });


        std::vector<uint16_t> indices;
        auto next = static_cast<uint16_t>(_geometry.size());
        uint16_t max = vertices.size() - next - 1;

        for (uint16_t ind = 0; ind < max; ++ind)
        {

            indices.push_back(ind);
            indices.push_back(ind + next);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + next + 1);
            indices.push_back(ind + 1);
            indices.push_back(ind);
        }


        auto vertArray = vsg::vec3Array::create(vertices.size());
        std::copy(vertices.begin(), vertices.end(), vertArray->begin());
        /*
        auto colorArray = vsg::vec3Array::create(vertices.size());
        std::fill(colorArray->begin(), colorArray->end(), vsg::vec3(1.0f, 1.0f, 1.0f));
        auto texArray = vsg::vec3Array::create(vertices.size());
        std::fill(texArray->begin(), texArray->end(), vsg::vec3(0.0f, 0.0f, 0.0f));
        */
        auto ind = vsg::ushortArray::create(indices.size());
        std::copy(indices.begin(), indices.end(), ind->begin());

        group->addChild(generateRails(vsg::DataList{vertArray}, ind));
        children.push_back(group);
    }

    vsg::ref_ptr<vsg::Node> SplineTrajectory::generateRails(vsg::DataList list, vsg::ref_ptr<vsg::ushortArray> indices)
    {
        /*
           // set up search paths to SPIRV shaders and textures
           vsg::Paths searchPaths = vsg::getEnvPaths("VSG_FILE_PATH");

           // load shaders
           vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", vsg::findFile("shaders/vert_PushConstants.spv", searchPaths));
           vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", vsg::findFile("shaders/frag_PushConstants.spv", searchPaths));
           if (!vertexShader || !fragmentShader)
           {
               return vsg::ref_ptr<vsg::Node>();
           }

           // read texture image
           vsg::Path textureFile("textures/lz.vsgb");
           auto textureData = vsg::read_cast<vsg::Data>(vsg::findFile(textureFile, searchPaths));
           if (!textureData)
           {
               return vsg::ref_ptr<vsg::Node>();
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
           auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

           auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{texture});
           auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

           // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
           auto scenegraph = vsg::StateGroup::create();
           scenegraph->add(bindGraphicsPipeline);
           scenegraph->add(bindDescriptorSet);

           // set up model transformation node
           //auto transform = vsg::MatrixTransform::create(); // VK_SHADER_STAGE_VERTEX_BIT

           // add transform to root of the scene graph
           //scenegraph->addChild(transform);

            set up vertex and index arrays
           auto vertices = vsg::vec3Array::create(
               {{-0.5f, -0.5f, 0.0f},
                {0.5f, -0.5f, 0.0f},
                {0.5f, 0.5f, 0.0f},
                {-0.5f, 0.5f, 0.0f},
                {-0.5f, -0.5f, -0.5f},
                {0.5f, -0.5f, -0.5f},
                {0.5f, 0.5f, -0.5f},
                {-0.5f, 0.5f, -0.5f}}); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_INSTANCE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

           auto colors = vsg::vec3Array::create(
               {
                   {1.0f, 0.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f},
                   {0.0f, 0.0f, 1.0f},
                   {1.0f, 1.0f, 1.0f},
                   {1.0f, 0.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f},
                   {0.0f, 0.0f, 1.0f},
                   {1.0f, 1.0f, 1.0f},
               }); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

           auto texcoords = vsg::vec2Array::create(
               {{0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f},
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f}}); // VK_FORMAT_R32G32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

           auto indices = vsg::ushortArray::create(
               {0, 1, 2,
                2, 3, 0,
                4, 5, 6,
                6, 7, 4}); // VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

           // setup geometry */
        auto stateGroup = _builder->createStateGroup();

        stateGroup->addChild(vsg::BindVertexBuffers::create(0, list));
        stateGroup->addChild(vsg::BindIndexBuffer::create(indices));
        stateGroup->addChild(vsg::DrawIndexed::create(indices->size(), 1, 0, 0, 0));

        _builder->compile(stateGroup);

        return stateGroup;
    }

    SceneTrajectory::SceneTrajectory()
        : vsg::Inherit<vsg::Node, SceneTrajectory>()
    {
    }
    SceneTrajectory::SceneTrajectory(Trajectory *traj)
        : SceneTrajectory()
    {
        traj = trajectory;
    }

    SceneTrajectory::~SceneTrajectory() {}

    void SceneTrajectory::read(vsg::Input& input)
    {
        Node::read(input);

        std::string name;
        input.read("trajName", name);

        trajectory = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);

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

    void SceneTrajectory::write(vsg::Output& output) const
    {
        Node::write(output);

        std::string name;
        trajectory->getValue(META_NAME, name);
        output.write("trajName", name);

        //output.write("files", files);
    }
}
