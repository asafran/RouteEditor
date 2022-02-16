#include "AddRails.h"
#include "ui_AddRails.h"
#include <vsg/nodes/Switch.h>
#include <vsg/io/read.h>
#include "ParentVisitor.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


AddRails::AddRails(DatabaseManager *database, QString root, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::AddRails)
{
    ui->setupUi(this);
    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    _fsmodel->setNameFilters(app::FORMATS);
    _fsmodel->setNameFilterDisables(false);
    ui->railView->setModel(_fsmodel);
    ui->sleeperView->setModel(_fsmodel);
    ui->fillView->setModel(_fsmodel);
    ui->railView->setRootIndex(_fsmodel->index(root + "/rails"));
    ui->sleeperView->setRootIndex(_fsmodel->index(root + "/sleepers"));
    ui->fillView->setRootIndex(_fsmodel->index(root + "/fill"));
}

AddRails::~AddRails()
{
    delete ui;
}
/*
tinyobj::ObjReader AddRails::loadObj(std::string path)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = path; // Path to material files

    tinyobj::ObjReader reader;

    reader.ParseFromFile(path, reader_config);
    //if (!reader.ParseFromFile(railFilepath.toStdString(), reader_config))
    //{
      //if (!reader.Error().empty())
          //return std::pair<attrib_t, std::string>();
    //}


    //auto& attrib = reader.GetAttrib();
    //auto& shapes = reader.GetShapes();
    //auto& materials = reader.GetMaterials();

    //auto name = materials.front().diffuse_texname;

    return reader;
}*/

void AddRails::intersection(const FindNode &isection)
{
    if(ui->railView->selectionModel()->selection().empty() ||
       ui->sleeperView->selectionModel()->selection().empty() ||
       ui->fillView->selectionModel()->selection().empty() )
        return;

    auto activeRail = ui->railView->selectionModel()->selectedIndexes().front();
    auto activeSleeper = ui->sleeperView->selectionModel()->selectedIndexes().front();
    auto activeFill = ui->fillView->selectionModel()->selectedIndexes().front();

    if(!activeRail.isValid() || !activeSleeper.isValid() || !activeFill.isValid())
        return;

    auto railFilepath = _fsmodel->filePath(activeRail);
    auto sleeperFilepath = _fsmodel->filePath(activeSleeper).toStdString();
    auto fillFilepath = _fsmodel->filePath(activeFill);

    vsg::ref_ptr<route::RailConnector> bwd;


    if(isection.connector && isection.connector->isFree())
    {
        if(ui->noNewBox->isChecked())
        {
            auto trj = isection.connector->trajectory ? isection.connector->trajectory : isection.connector->fwdTrajectory;
            if(auto strj = trj->cast<route::SplineTrajectory>(); strj)
            {
                auto point = route::RailPoint::create(*isection.connector);
                strj->add(point);
                emit sendMovingPoint(isection.connector);
                emit startMoving();
                return;
            }
        }
        bwd = isection.connector;
    }
    else
        bwd = route::RailConnector::create(_database->getStdAxis(), _database->getStdWireBox(), isection.worldIntersection);

    auto fwd = route::RailConnector::create(_database->getStdAxis(), _database->getStdWireBox(), isection.worldIntersection);

    auto sleeper = vsg::read_cast<vsg::Node>(sleeperFilepath, _database->builder->options);

    if(!sleeper)
        return;

    _database->builder->compileTraversal->compile(sleeper);

    auto traj = route::SplineTrajectory::create("trj",
                                                bwd,
                                                fwd,
                                                _database->builder,
                                                railFilepath.toStdString(), fillFilepath.toStdString(),
                                                sleeper, 2.0, 1.5);
    auto adapter = route::SceneTrajectory::create(traj);

    if(!isection.tile)
        return;
    auto tilesModel = _database->tilesModel;
    auto index = tilesModel->index(isection.tile);

    _database->topology->insertTraj(traj);

    ParentIndexer pi;
    adapter->accept(pi);

    _database->undoStack->push(new AddSceneObject(tilesModel, index, adapter));

    emit sendMovingPoint(fwd);
    emit startMoving();
}
