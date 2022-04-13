#include "Painter.h"
#include "ui_Painter.h"
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/io/read.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <QImage>


Painter::Painter(DatabaseManager *database, QWidget *parent) :
    Tool(database, parent),
    ui(new Ui::Painter)
{
    ui->setupUi(this);
}

Painter::~Painter()
{
    delete ui;
}

void Painter::intersection(const FoundNodes &isection)
{
    isection.trajectory->setBusy();


    /*
    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(_database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
    if(!isection.terrain || !ellipsoidModel)
        return;
    vsg::ComputeBounds cb;
    cb.traversalMask = 0x0;
    isection.terrain->children.front()->accept(cb);
    auto matrix= isection.terrain->matrix;

    struct FindTexture : public vsg::Visitor
    {
        //vsg::ref_ptr<vsg::ImageInfo> imageInfo;
        vsg::ref_ptr<vsg::Data> texture;

        void apply(vsg::Object& object) override
        {
            object.traverse(*this);
        }
        void apply(vsg::StateGroup& sg) override
        {
            // create texture image and associated DescriptorSets and binding
            auto descriptorTexture = vsg::DescriptorImage::create(vsg::Sampler::create(), texture, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            auto pipeline = sg.stateCommands.front().cast<vsg::BindGraphicsPipeline>()->pipeline;
            auto pipelineLayout = pipeline->layout;

            vsg::DescriptorSetLayoutBindings descriptorBindings{
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
            };

            auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

            pipelineLayout->setLayouts.clear();
            pipelineLayout->setLayouts.push_back(descriptorSetLayout);

            auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{descriptorTexture});
            auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

            //sg.add(bindDescriptorSet);

            //sg.traverse(*this);
        }

        void apply(vsg::CullNode& cn) override
        {
            auto group = cn.child.cast<vsg::StateGroup>();
            auto sg = route::createStateGroup(texture);
            sg->addChild(group->children.front());
            cn.child = sg;

            sg->traverse(*this);
            ct->compile(&cn);
        }
        void apply(vsg::VertexIndexDraw& vdi) override
        {
            vdi.arrays.erase(std::next(vdi.arrays.begin()));
        }

        void apply(vsg::DescriptorImage& di) override
        {
            if (!di.imageInfoList.empty()) imageInfo = di.imageInfoList[0]; // contextID=0, and only one imageData
        }

        static void find(const vsg::Node* node, vsg::ref_ptr<vsg::Data> texture)
        {
            FindTexture fdi;
            fdi.texture = texture;
            fdi.traversalMask = 0x0;
            const_cast<vsg::Node*>(node)->accept(fdi);
            //return fdi;
        }
    };

    FindTexture::find(isection.terrain, vsg::read_cast<vsg::Data>("/home/asafr/RRS/objects/rails/rails/Grid.png", _database->builder->options));
    _database->builder->compileTraversal->compile(const_cast<vsg::MatrixTransform*>(isection.terrain));

    if(!textureImageInfo)
    {
        vsg::Path textureFile("textures/lz.vsgb");
        auto textureData = vsg::read_cast<vsg::Data>(textureFile, _database->builder->options);
        if (!textureData)
            return;

    }

    auto image = new QImage();
*
    vsg::vec3 centre((cb.bounds.min + cb.bounds.max) * 0.5);

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    state.wireframe = true;
    state.lighting = false;

    auto delta = cb.bounds.max - cb.bounds.min;

    info.dx.set(delta.x, 0.0f, 0.0f);
    info.dy.set(0.0f, delta.y, 0.0f);
    info.dz.set(0.0f, 0.0f, delta.z);
    info.position = centre;
    info.transform = isection.terrain->matrix;
    auto builder = _database->getBuilder();
    _database->getRoot()->addChild(builder->createBox(info, state));

    const_cast<vsg::MatrixTransform*>(isection.terrain)->children.clear();

    auto min = ellipsoidModel->convertECEFToLatLongAltitude(matrix * cb.bounds.min);
    auto max = ellipsoidModel->convertECEFToLatLongAltitude(matrix * cb.bounds.max);
    auto position = ellipsoidModel->convertECEFToLatLongAltitude(isection.worldIntersection) - min;
    auto size = max - min;
    volatile auto xcoeff = position.x / size.x;
    volatile auto ycoeff = position.y / size.y;
    */
}
