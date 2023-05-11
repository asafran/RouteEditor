#include "ContentManager.h"
#include "sceneobjects.h"
#include "sceneobjectvisitor.h"
#include "ui_ContentManager.h"
#include <QSettings>
#include <vsg/io/read.h>
#include "DatabaseManager.h"
#include "trajectory.h"
#include "undo-redo.h"
#include <vsg/app/Viewer.h>
#include <vsg/threading/OperationThreads.h>
#include "signals.h"
#include "tile.h"

ContentManager::ContentManager(DatabaseManager *database, QString root, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::ContentManager)
{
    ui->setupUi(this);
    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    _fsmodel->setNameFilters(app::FORMATS);
    _fsmodel->setNameFilterDisables(false);
    ui->fileView->setModel(_fsmodel);
    ui->fileView->setRootIndex(_fsmodel->index(root));
}
/*
bool ContentManager::addToTrack(vsg::ref_ptr<route::SceneObject> obj, const FoundNodes &isection)
{
    if(!isection.trajectory)
        return false;
    auto traj = isection.trajectory->cast<route::StraitTrajectory>();
    if(!traj)
        return false;
    auto coord = traj->invert(isection.intersection->worldIntersection);
    //obj->recalculateWireframe();
    auto transform = vsg::MatrixTransform::create();
    obj->setValue(app::PARENT, transform.get());
    transform->addChild(obj);
    transform->setValue(app::PROP, coord);
    obj->reset();
    auto model = _database->tilesModel;
    _database->undoStack->push(new AddSceneObject(model, model->index(traj), transform));
    traj->updateAttached();
    emit sendObject(obj);
    return true;
}

bool ContentManager::addSignal(vsg::ref_ptr<route::SceneObject> obj, const FoundNodes &isection)
{
    auto sig = obj.cast<signalling::Signal>();
    if(!isection.connector || !sig)
        return false;
    if(ui->reverseBox->isChecked())
        sig->setRotation(vsg::dquat(vsg::PI, vsg::dvec3(0.0, 0.0, 1.0)));
    if(auto ab = sig.cast<signalling::AutoBlockSignal>(); ab)
        ab->fstate = ui->fstateBox->isChecked();

    bool connect = ui->connectBox->isChecked();
    if(!isection.connector->fwdSignal() && !ui->reverseBox->isChecked())
        _database->undoStack->push(new AddFwdSignal(isection.connector, sig, _database->topology, connect));
    else if(!isection.connector->bwdSignal() && ui->reverseBox->isChecked())
        _database->undoStack->push(new AddBwdSignal(isection.connector, sig, _database->topology, connect));
    return true;
}
*/
void ContentManager::activeGroupChanged(const QModelIndex &index)
{
    _activeGroup = index;
}

ContentManager::~ContentManager()
{
    delete ui;
}


void ContentManager::apply(vsg::ButtonPressEvent &buttonPress)
{
    if(!isVisible() || buttonPress.button != 1 || ui->fileView->selectionModel()->selectedIndexes().empty())
        return;
    auto isections = route::testIntersections(buttonPress, _database->root, _camera);
    if(isections.empty())
        return;
    auto isection = isections.front();

    bool loadToSelected = !ui->autoGroup->isChecked();
    auto activeFile = ui->fileView->selectionModel()->selectedIndexes().front();
    if(!activeFile.isValid())
    {
        emit sendStatusText(tr("Выберите файл"), 2000);
        return;
    }
    else if(!_activeGroup.isValid() && loadToSelected)
    {
        emit sendStatusText(tr("Выберите группу для объекта"), 2000);
        return;
    }
    auto path = _fsmodel->filePath(activeFile).toStdString();

    auto load = [database=_database, useLinks=ui->useLinks->isChecked(), path]()
    {
        vsg::ref_ptr<route::SceneObject> object;
        auto node = vsg::read_cast<vsg::Node>(path, database->builder->options);
        if(!node)
            return object;

        if(object = node->cast<route::SceneObject>(); object)
            return object;

        if(useLinks)
            object = route::SingleLoader::create(database->getStdWireBox(), node, path);
        else
            object = route::SceneObject::create(database->getStdWireBox(), node);

        return object;
    };

    auto add = [this, type=ui->typeBox->currentIndex(), loadToSelected=!ui->autoGroup->isChecked(), isection](vsg::ref_ptr<route::SceneObject> object)
    {
        auto model = _database->tilesModel;
        if(!object)
            return;
        if(loadToSelected)
        {
            auto group = static_cast<route::MVCObject*>(_activeGroup.internalPointer());
            auto ltw = group->getWorldTransform();
            auto wtl = vsg::inverse(ltw);
            auto world = isection->worldIntersection;

            object->setPosition(world * wtl);

            _database->undoStack->push(new AddSceneObject(model, _activeGroup, object));
        }
        else
        {
            route::AddCast visitor;
            visitor.tileFunction = [this, model, object, &isection, type](route::Tile *tile)
            {
                auto model = _database->tilesModel;
                auto index = model->index(type, 0, model->index(tile));
                auto world = isection->worldIntersection;
                auto position = vsg::inverse(tile->transform->matrix) * world;
                object->setPosition(position);
                _database->undoStack->push(new AddSceneObject(model, index, object));
                return true;
            };
            visitor.trjFunction = [this, model, object, &isection](route::Trajectory *traj)
            {
                auto coord = traj->invert(isection->worldIntersection);
                auto trjObject = route::TrajectoryObject::create();
                trjObject->coord = coord;
                trjObject->addChild(object);
                _database->undoStack->push(new AddSceneObject(model, model->index(traj), trjObject));
                traj->updateAttached();
                return true;
            };
            vsg::visit(visitor, isection->nodePath);
        }
    };

    auto loadOperation = LoadOperation<decltype(load), decltype(add)>::create(_database->viewer, load, add);

    _database->opThreads->add(loadOperation);


/*
    if(ui->removeButt->isChecked())
    {
        if(isection.connector)
        {
            if(ui->reverseBox->isChecked() && isection.connector->bwdSignal())
                _database->undoStack->push(new RemoveBwdSignal(isection.connector, _database->topology));
            else if (!ui->reverseBox->isChecked() && isection.connector->fwdSignal())
                _database->undoStack->push(new RemoveFwdSignal(isection.connector, _database->topology));
        } else if(!isection.objects.empty())
        {
            auto index = _database->tilesModel->index(isection.objects.front());
            if(index.isValid())
                _database->undoStack->push(new RemoveNode(_database->tilesModel, index));
        }
    } else
        auto future = QtConcurrent::run(load).then(this, add);*/
}

