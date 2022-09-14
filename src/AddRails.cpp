#include "AddRails.h"
#include "ui_AddRails.h"
#include <vsg/nodes/Switch.h>
#include <vsg/io/read.h>
#include "ParentVisitor.h"
#include <vsg/viewer/Viewer.h>


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

void AddRails::intersection(const FoundNodes &isection)
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
        auto trj = isection.connector->trajectory ? isection.connector->trajectory : isection.connector->fwdTrajectory;
        bwd = isection.connector;

        if(auto strj = trj->cast<route::SplineTrajectory>(); strj)
        {
            if(ui->noNewBox->isChecked())
            {
                auto point = route::RailPoint::create(*isection.connector);
                _database->undoStack->push(new AddRailPoint(strj, point));
                emit sendMovingPoint(isection.connector);
                emit startMoving();
                return;
            }
        } else
        {
            trj->_fwdPoint->staticConnector = true;
            trj->_bwdPoint->staticConnector = true;
        }
    }
    else
        bwd = route::RailConnector::create(_database->getStdAxis(), _database->getStdWireBox(), isection.intersection->worldIntersection);

    auto fwd = route::RailConnector::create(_database->getStdAxis(), _database->getStdWireBox(), isection.intersection->worldIntersection);

    auto sleeper = vsg::read_cast<vsg::Node>(sleeperFilepath, _database->builder->options);

    if(!sleeper)
        return;

    auto result = _database->viewer->compileManager->compile(sleeper);
    vsg::updateViewer(*_database->viewer, result);

    vsg::ref_ptr<route::Trajectory> traj;

    double gaudge = static_cast<double>(ui->gaudgeSpin->value()) / 1000.0;
    double slpr = ui->slpSpin->value();

    if(ui->genBox->isChecked())
    {
        auto lenght = ui->lenghtSpin->value();
        fwd->setPosition((vsg::normalize(bwd->getTangent()) * lenght) + bwd->getPosition() + (vsg::normalize(bwd->getPosition()) * ui->altBox->value()));
        traj = route::StraitTrajectory::create("trajectory",
                                               bwd,
                                               fwd,
                                               _database->builder->options,
                                               railFilepath.toStdString(), fillFilepath.toStdString(),
                                               sleeper, slpr, gaudge);
    }
    else
    {
        traj = route::SplineTrajectory::create("trajectory",
                                               bwd,
                                               fwd,
                                               _database->builder->options,
                                               railFilepath.toStdString(), fillFilepath.toStdString(),
                                               sleeper, slpr, gaudge);
        emit sendMovingPoint(fwd);
        emit startMoving();
    }

    traj->recalculate();

    _database->undoStack->push(new AddSceneObject(_database->tilesModel, _database->root, traj));
}
