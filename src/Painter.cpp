#include "Painter.h"
#include "ui_Painter.h"
#include <vsg/utils/ComputeBounds.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/io/read.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <QImage>
#include <QPainter>

#include "tile.h"


Painter::Painter(DatabaseManager *database, QString root, QWidget *parent) :
    Tool(database, parent),
    ui(new Ui::Painter),
    _alpha(0, 0, 128)
{
    ui->setupUi(this);

    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    ui->fileView->setModel(_fsmodel);
    ui->fileView->setRootIndex(_fsmodel->index(root));

    _alpha.setColorAt(0.0, Qt::transparent);
    _alpha.setColorAt(1, Qt::white);

    connect(ui->fileView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Painter::activeTextureChanged);
}

Painter::~Painter()
{
    delete ui;
}

void Painter::intersection(const FoundNodes &isection)
{
    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(_database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
    if(!isection.tile || !ellipsoidModel)
        return;

    auto &terrain = isection.tile->terrain;
    auto &texture = isection.tile->texture;

    QImage::Format format;

    switch (texture->properties.format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
        format = QImage::Format_RGBA8888;
        break;
    default:
        return;
    }

    auto transform = terrain->getObject<vsg::doubleArray>("GeoTransform");
    if(!transform)
        return;
    auto qimage = new QImage(static_cast<uchar*>(texture->dataPointer()), texture->width(), texture->height(), format);
    //potential memory leak
    auto origin = vsg::dvec3(transform->at(0), transform->at(3), 0.0);
    auto dx = transform->at(1) * terrain->width();
    auto dy =  transform->at(5) * terrain->height();

    auto lla = ellipsoidModel->convertECEFToLatLongAltitude(isection.intersection->worldIntersection);
    auto delta = lla - origin;
    auto u = delta.x / dx;
    auto v = delta.y / dy;

    QPoint point(static_cast<int>(texture->width() * u), static_cast<int>(texture->height() * v));

    QPainter p(qimage);

    p.setPen(Qt::NoPen);

    QRect rect(0, 0, _size, _size);
    rect.moveCenter(point);

    _alpha.setCenter(point);
    _alpha.setFocalPoint(point);
    _alpha.setCenterRadius(_size / 2);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.setBrush(_alpha);
    p.drawRect(rect);

    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    p.drawImage(rect, _image);

    texture->dirty();

    //delete qimage;
    //terrain->dirty();

     //_database->copyImageCmd->copy(texture, fdi.imageInfo);
}

void Painter::activeTextureChanged(const QItemSelection &selected, const QItemSelection &)
{
    auto index = selected.indexes().front();
    auto path = _fsmodel->filePath(index);
    _image = QImage(path);
}
