#include    "trajectory.h"

#include    <QFile>
#include    <QDir>
#include    <QTextStream>
#include    <vsg/io/read.h>
#include    "topology.h"


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

SplineTrajectory::SplineTrajectory(std::string name, const vsg::dvec3 &lla_point)
  : vsg::Inherit<Trajectory, SectionTrajectory>()
{
    std::vector<vsg::dvec2> vec2vector;
    vec2vector.emplace_back(lla_point.x, lla_point.y);
    railSpline.reset(new UniformCRSpline<vsg::dvec2>(vec2vector));
}

SplineTrajectory::SplineTrajectory()
  : vsg::Inherit<Trajectory, SectionTrajectory>()
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

void SplineTrajectory::recalculate()
{

}

void SplineTrajectory::applyChanges(vsg::ref_ptr<vsg::vec3Array> vertices, vsg::ref_ptr<vsg::vec3Array> colors, vsg::ref_ptr<vsg::vec2Array> texcoords, vsg::ref_ptr<vsg::ushortArray> indices)
{

    // set up defaults and read command line arguments to override them
       vsg::CommandLine arguments(&argc, argv);

       auto windowTraits = vsg::WindowTraits::create();
       windowTraits->debugLayer = arguments.read({"--debug", "-d"});
       windowTraits->apiDumpLayer = arguments.read({"--api", "-a"});
       arguments.read({"--window", "-w"}, windowTraits->width, windowTraits->height);

       if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);

       // set up search paths to SPIRV shaders and textures
       vsg::Paths searchPaths = vsg::getEnvPaths("VSG_FILE_PATH");

       // load shaders
       vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", vsg::findFile("shaders/vert_PushConstants.spv", searchPaths));
       vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", vsg::findFile("shaders/frag_PushConstants.spv", searchPaths));
       if (!vertexShader || !fragmentShader)
       {
           std::cout << "Could not create shaders." << std::endl;
           return 1;
       }

       // read texture image
       vsg::Path textureFile("textures/lz.vsgb");
       auto textureData = vsg::read_cast<vsg::Data>(vsg::findFile(textureFile, searchPaths));
       if (!textureData)
       {
           std::cout << "Could not read texture file : " << textureFile << std::endl;
           return 1;
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
       auto transform = vsg::MatrixTransform::create(); // VK_SHADER_STAGE_VERTEX_BIT

       // add transform to root of the scene graph
       scenegraph->addChild(transform);

       /* set up vertex and index arrays
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
       auto drawCommands = vsg::Commands::create();
       drawCommands->addChild(vsg::BindVertexBuffers::create(0, vsg::DataList{vertices, colors, texcoords}));
       drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
       drawCommands->addChild(vsg::DrawIndexed::create(12, 1, 0, 0, 0));
}

SceneTrajectory::SceneTrajectory()
    : vsg::Inherit<vsg::Node, SceneTrajectory>()
{
}
SceneTrajectory::SceneTrajectory(Trajectory *trajectory)
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

    traj = input.options->objectCache->get(TOPOLOGY_KEY).cast<Topology>()->trajectories.at(name);

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
    traj->getValue(META_NAME, name);
    output.write("trajName", name);

    //output.write("files", files);
}

