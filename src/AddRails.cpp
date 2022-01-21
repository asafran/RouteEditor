#include "AddRails.h"
#include "ui_AddRails.h"
#include <vsg/nodes/Switch.h>

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
    if(!activeFile.isValid())
        return;

    auto path = _fsmodel->filePath(activeFile).toStdString();
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
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

    std::vector<vsg::vec3> geometry;
    std::vector<vsg::ref_ptr<route::RailPoint>> points;

    auto it = attrib.vertices.begin();
    auto end = attrib.vertices.end() - (attrib.vertices.size()/2);
    while (it < end)
        geometry.push_back(vsg::vec3(*it++, *it++, *it++));

    auto point = _database->getBuilder()->createSphere();

    auto bwd = route::RailConnector::create(isection.worldIntersection, point);
    auto fwd = route::RailConnector::create(isection.worldIntersection + vsg::dvec3(30.0, 0.0, 0.0), point);

    auto traj = route::SplineTrajectory::create("trj", bwd, fwd, _database->getBuilder(), geometry, _database->getBuilder()->createBox(), 0.5);
    auto adapter = route::SceneTrajectory::create(traj);

    const_cast<vsg::Switch*>(isection.tile.first)->addChild(route::SceneObjects, adapter);
}
