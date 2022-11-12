#include "ContentManager.h"
#include "sceneobjects.h"
#include "ui_ContentManager.h"
#include <QSettings>
#include <vsg/io/read.h>
#include "ParentVisitor.h"
#include "DatabaseManager.h"
#include <vsg/viewer/Viewer.h>
#include "signals.h"

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

void ContentManager::intersection(const FoundNodes &isection)
{
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

    auto load = [database=_database, activeGroup=_activeGroup, loadToSelected=!ui->autoGroup->isChecked(), useLinks=ui->useLinks->isChecked(), path, isection]()
    {
        std::pair<vsg::ref_ptr<route::SceneObject>, vsg::CompileResult> loaded;
        auto node = vsg::read_cast<vsg::Node>(path, database->builder->options);
        if(!node)
            return loaded;

        loaded.second = database->viewer->compileManager->compile(node);

        if(auto object = node->cast<route::SceneObject>(); object)
        {
            loaded.first = object;
            return loaded;
        }

        vsg::dmat4 ltw;
        vsg::dquat wquat;
        auto world = isection.intersection->worldIntersection;
        auto norm = vsg::normalize(world);

        if(loadToSelected)
        {
            auto group = static_cast<vsg::Node*>(activeGroup.internalPointer());

            ParentTracer pt;
            group->accept(pt);
            pt.nodePath.push_back(group);
            ltw = vsg::computeTransform(pt.nodePath);
        }
        else
            wquat = vsg::dquat(vsg::dvec3(0.0, 0.0, 1.0), norm);

        auto wtl = vsg::inverse(ltw);

        if(useLinks)
            loaded.first = route::SingleLoader::create(node, database->getStdWireBox(), path, wtl * world, wquat, ltw);
        else
            loaded.first = route::SceneObject::create(node, database->getStdWireBox(), wtl * world, wquat, ltw);

        return loaded;
    };

    auto add = [this, isection, loadToSelected=!ui->autoGroup->isChecked(), path](std::pair<vsg::ref_ptr<route::SceneObject>, vsg::CompileResult> obj)
    {
        if(!obj.first)
        {
            emit sendStatusText(tr("Ошибка чтения файла %1").arg(path.c_str()), 2000);
            return;
        }

        if(addToTrack(obj.first, isection))
        {
            emit sendStatusText(tr("Объект привязян к траектории"), 2000);
            return;
        }

        if(addSignal(obj.first, isection))
        {
            emit sendStatusText(tr("Установлен сигнал"), 2000);
            return;
        }

        QModelIndex activeGroup;

        if(loadToSelected)
        {
            activeGroup = _activeGroup;
            auto group = static_cast<vsg::Node*>(_activeGroup.internalPointer());

            ParentTracer pt;
            group->accept(pt);
            pt.nodePath.push_back(group);
            obj.first->localToWorld = vsg::inverse(vsg::computeTransform(pt.nodePath));
            obj.first->recalculateWireframe();
        }
        else if(isection.tile)
            activeGroup = _database->tilesModel->index(isection.tile);
        else
        {
            emit sendStatusText(tr("Кликните по тайлу"), 2000);
            return;
        }

        updateViewer(*_database->viewer, obj.second);

        _database->undoStack->push(new AddSceneObject(_database->tilesModel, activeGroup, obj.first));
        emit sendObject(obj.first);
        emit sendStatusText(tr("Добавлен объект"), 2000);
    };

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
        auto future = QtConcurrent::run(load).then(this, add);
}

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
    obj->world_quat = {0.0, 0.0, 0.0, 1.0};
    obj->setPosition({0.0, 0.0, 0.0});
    auto model = _database->tilesModel;
    _database->undoStack->push(new AddSceneObject(model, model->index(traj), transform));
    traj->updateAttached();
    ApplyTransform ct;
    ct.stack.push(vsg::dmat4());
    traj->accept(ct);
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

void ContentManager::activeGroupChanged(const QModelIndex &index)
{
    _activeGroup = index;
}

ContentManager::~ContentManager()
{
    delete ui;
}
