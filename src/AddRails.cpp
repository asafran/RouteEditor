#include "AddRails.h"
#include "ui_AddRails.h"
#include <vsg/nodes/Switch.h>
#include <vsg/io/read.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


AddRails::AddRails(DatabaseManager *database, QString root, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::AddRails)
{
    ui->setupUi(this);
    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    ui->fileView->setModel(_fsmodel);
    ui->fileView->setRootIndex(_fsmodel->index(root));
}

AddRails::~AddRails()
{
    delete ui;
}

void AddRails::intersection(const FindNode &isection)
{
    auto activeFile = ui->fileView->selectionModel()->selectedIndexes().front();
    if(ui->fileView->selectionModel()->selection().empty() || !activeFile.isValid())
        return;

    auto filepath = _fsmodel->filePath(activeFile);
    QFileInfo fi(filepath);
    auto path = fi.absolutePath().toStdString();

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = path; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filepath.toStdString(), reader_config)) {
      if (!reader.Error().empty()) {
          std::cerr << "TinyObjReader: " << reader.Error();
      }
      return;
    }

    if (!reader.Warning().empty()) {
      std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    auto name = materials.front().diffuse_texname;

    auto texture = vsg::read_cast<vsg::Data>(path + "/" + name, _database->getBuilder()->options);

    std::vector<vsg::ref_ptr<route::RailPoint>> points;

    auto builder = _database->getBuilder();

    auto group = vsg::Group::create();
    vsg::GeometryInfo gi;
    gi.dx = vsg::vec3(1.0f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    group->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 1.0f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    group->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 1.0f);
    gi.color = vsg::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    group->addChild(builder->createBox(gi));

    auto bwd = route::RailConnector::create(group, isection.worldIntersection);
    auto fwd = route::RailConnector::create(builder->createCylinder(), isection.worldIntersection + vsg::dvec3(30.0, -20.0, 00.0));

    auto traj = route::SplineTrajectory::create("trj", bwd, fwd, builder, attrib, texture, group, 2);
    auto adapter = route::SceneTrajectory::create(traj);

    const_cast<vsg::Switch*>(isection.tile.first)->addChild(route::SceneObjects, adapter);

    auto point = route::RailPoint::create(group, isection.worldIntersection + vsg::dvec3(10.0, -10.0, 20.0));
    /*
    auto bwd = route::RailConnector::create(group, isection.worldIntersection);
    auto fwd = route::RailConnector::create(builder->createCylinder(), isection.worldIntersection + vsg::dvec3(-30.0, -20.0, 20.0));

    auto traj = route::SplineTrajectory::create("trj", bwd, fwd, builder, attrib, texture, group, 2);
    auto adapter = route::SceneTrajectory::create(traj);

    const_cast<vsg::Switch*>(isection.tile.first)->addChild(route::SceneObjects, adapter);

    auto point = route::RailPoint::create(group, isection.worldIntersection + vsg::dvec3(-10.0, -10.0, 20.0));
    */
    traj->add(point);
}
