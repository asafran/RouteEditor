#include "Painter.h"
#include "ui_Painter.h"
#include <vsg/traversals/ComputeBounds.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/io/read.h>
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

void Painter::intersection(const FindNode &isection)
{
    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(_database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
    if(!isection.terrain || !ellipsoidModel)
        return;
    vsg::ComputeBounds cb;
    cb.traversalMask = 0x0;
    isection.terrain->children.front()->accept(cb);
    auto matrix= isection.terrain->matrix;

    struct FindTexture : public vsg::Visitor
    {
        vsg::ref_ptr<vsg::ImageInfo> imageInfo;

        void apply(vsg::Object& object) override
        {
            object.traverse(*this);
        }
        void apply(vsg::StateGroup& sg) override
        {
            for (auto& sc : sg.stateCommands) { sc->accept(*this); }
            sg.traverse(*this);
        }
        void apply(vsg::DescriptorImage& di) override
        {
            if (!di.imageInfoList.empty()) imageInfo = di.imageInfoList[0]; // contextID=0, and only one imageData
        }

        static vsg::ref_ptr<vsg::ImageInfo> find(const vsg::Node* node)
        {
            FindTexture fdi;
            fdi.traversalMask = 0x0;
            const_cast<vsg::Node*>(node)->accept(fdi);
            return fdi.imageInfo;
        }
    };

    auto textureImageInfo = FindTexture::find(isection.terrain);

    if(!textureImageInfo)
    {
        vsg::Path textureFile("textures/lz.vsgb");
        auto textureData = vsg::read_cast<vsg::Data>(textureFile, _database->builder->options);
        if (!textureData)
            return;

    }

    auto image = new QImage();

/*
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
*/
    auto min = ellipsoidModel->convertECEFToLatLongAltitude(matrix * cb.bounds.min);
    auto max = ellipsoidModel->convertECEFToLatLongAltitude(matrix * cb.bounds.max);
    auto position = ellipsoidModel->convertECEFToLatLongAltitude(isection.worldIntersection) - min;
    auto size = max - min;
    volatile auto xcoeff = position.x / size.x;
    volatile auto ycoeff = position.y / size.y;
}
