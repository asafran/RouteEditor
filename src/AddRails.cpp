#include "AddRails.h"
#include "ui_AddRails.h"

#include "sceneobjectvisitor.h"
#include "undo-redo.h"

#include <vsg/nodes/Switch.h>
#include <vsg/io/read.h>
#include <vsg/app/Viewer.h>

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

void AddRails::apply(vsg::ButtonPressEvent &buttonPress)
{
    if(!isVisible() || buttonPress.button != 1)
        return;
    auto isections = route::testIntersections(buttonPress, _database->root, _camera);
    if(isections.empty())
        return;
    auto isection = isections.front();

    auto connector = vsg::visit<route::SceneObjectCast<route::Connector>>(isection->nodePath).ptr;

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

    if(connector && connector->isFree())
    {
        auto trj = connector->connected();
        if(auto strj = trj->cast<route::SplineTrajectory>(); strj)
        {
            if(ui->noNewBox->isChecked())
            {
                auto point = route::RailPoint::create(*connector);
                _database->undoStack->push(new AddRailPoint(strj, point));
                emit sendMovingPoint(connector);
                return;
            }
        } else
        {
            //trj->_fwdPoint->staticConnector = true;
            //trj->_bwdPoint->staticConnector = true;
        }
    }

    auto worldTolocal = vsg::inverse(_database->route->topology->transform->matrix);
    auto local = isection->worldIntersection * worldTolocal;

    auto front = route::BeginConnector::create(_database->getStdWireBox(), _database->getStdAxis(), local);
    auto back = route::EndConnector::create(_database->getStdWireBox(), _database->getStdAxis(), local);

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
        back->setPosition((vsg::normalize(front->getTangent()) * lenght) + front->getPosition() + (vsg::normalize(front->getPosition()) * ui->altBox->value()));
        traj = route::StraitTrajectory::create(front,
                                               back,
                                               _database->builder->options,
                                               railFilepath.toStdString(), fillFilepath.toStdString(),
                                               sleeper, slpr, gaudge);
    }
    else
    {
        traj = route::SplineTrajectory::create(front,
                                               back,
                                               _database->builder->options,
                                               railFilepath.toStdString(), fillFilepath.toStdString(),
                                               sleeper, slpr, gaudge);
    }

    traj->recalculate();

    _database->undoStack->push(new AddSceneObject(_database->tilesModel, _database->route->topology, traj));
}
