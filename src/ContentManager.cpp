#include "ContentManager.h"
#include "sceneobjects.h"
#include "ui_ContentManager.h"
#include <QSettings>
#include <vsg/io/read.h>
#include "ParentVisitor.h"
#include "DatabaseManager.h"

ContentManager::ContentManager(DatabaseManager *database, QString root, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::ContentManager)
  , modelsDir(root)
{
    ui->setupUi(this);
    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    ui->fileView->setModel(_fsmodel);
    ui->fileView->setRootIndex(_fsmodel->index(root));
}

void ContentManager::intersection(const FindNode &isection)
{
    bool loadToSelected = !ui->autoGroup->isChecked();
    auto activeFile = ui->fileView->selectionModel()->selectedIndexes().front();
    if(!activeFile.isValid() || (!_activeGroup.isValid() && loadToSelected))
        return;

    auto path = _fsmodel->filePath(activeFile).toStdString();
    auto node = vsg::read_cast<vsg::Node>(path);
    _database->builder->compile(node);

    if(addToTrack(node, isection))
        return;

    QModelIndex activeGroup;

    vsg::dmat4 wtl;
    auto world = isection.worldIntersection;

    if(loadToSelected)
    {
        activeGroup = _activeGroup;
        auto group = static_cast<vsg::Node*>(_activeGroup.internalPointer());

        ParentTracer pt;
        group->accept(pt);
        wtl = vsg::inverse(vsg::computeTransform(pt.nodePath));
    }
    else if(isection.tile)
        activeGroup = _database->tilesModel->index(isection.tile);
    else
        return;

    vsg::ref_ptr<route::SceneObject> obj;
    auto norm = vsg::normalize(world);
    vsg::dquat wquat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(ui->useLinks->isChecked())
        obj = route::SingleLoader::create(node, _database->getStdWireBox(), path, wtl * world, wquat, wtl);
    else
        obj = route::SceneObject::create(node, _database->getStdWireBox(), wtl * world, wquat, wtl);
    _database->undoStack->push(new AddSceneObject(_database->tilesModel, activeGroup, obj));
    emit sendStatusText(tr("Добавлен объект %1").arg(path.c_str()), 2000);
}

bool ContentManager::addToTrack(vsg::ref_ptr<vsg::Node> node, const FindNode &isection)
{
    auto traj = isection.track;
    if(!traj)
        return false;
    auto coord = traj->invert(isection.worldIntersection);
    auto obj = route::SceneObject::create(node, _database->getStdWireBox());
    auto transfrom = vsg::MatrixTransform::create();
    transfrom->addChild(obj);
    transfrom->setValue(META_PROPERTY, coord);
    auto model = _database->tilesModel;
    _database->undoStack->push(new AddSceneObject(_database->tilesModel, model->index(isection.track), obj));
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
