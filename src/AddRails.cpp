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

    auto bwd = route::RailConnector::create(isection.worldIntersection, vsg::Node::create());
    auto fwd = route::RailConnector::create(isection.worldIntersection + vsg::dvec3(30.0, 0.0, 0.0), vsg::Node::create());

    auto traj = route::SplineTrajectory::create("trj", bwd, fwd, _database->getBuilder(), attrib, texture, vsg::Node::create(), 3);
    auto adapter = route::SceneTrajectory::create(traj);

    const_cast<vsg::Switch*>(isection.tile.first)->addChild(route::SceneObjects, adapter);
}
