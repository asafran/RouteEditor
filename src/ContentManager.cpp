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
    _database->compile(node);

    if(addToTrack(node, isection))
        return;

    QModelIndex activeGroup = findGroup(isection);
    vsg::dmat4 wtl;
    auto world = isection.worldIntersection;

    if(loadToSelected || (_activeGroup.isValid() && !activeGroup.isValid()))
    {
        activeGroup = _activeGroup;
        auto group = static_cast<vsg::Node*>(_activeGroup.internalPointer());

        ParentVisitor pv(group);
        _database->getRoot()->accept(pv);
        pv.pathToChild.pop_back();
        wtl = vsg::inverse(vsg::computeTransform(pv.pathToChild));
    } else if(!_activeGroup.isValid() && !activeGroup.isValid())
        return;

    vsg::ref_ptr<route::SceneObject> obj;
    auto norm = vsg::normalize(world);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(ui->useLinks->isChecked())
        obj = route::SingleLoader::create(node, path, wtl * world, quat, wtl);
    else
        obj = route::SceneObject::create(node, wtl * world, quat, wtl);
    _database->push(new AddSceneObject(_database->getTilesModel(), activeGroup, obj));
    emit sendStatusText(tr("Добавлен объект %1").arg(path.c_str()), 2000);
}

QModelIndex ContentManager::findGroup(const FindNode &isection)
{
    if(isection.tile.first)
    {
        auto tilesModel = _database->getTilesModel();
        FindPositionVisitor fpv(isection.tile.first);
        auto index = tilesModel->index(fpv(tilesModel->getRoot()), 0, QModelIndex());
        return index;
    }
    return QModelIndex();
}

bool ContentManager::addToTrack(vsg::ref_ptr<vsg::Node> node, const FindNode &isection)
{
    auto traj = isection.track.first;
    if(!traj)
        return false;
    auto coord = traj->invert(isection.worldIntersection);
    auto obj = route::SceneObject::create(node);
    auto transfrom = vsg::MatrixTransform::create();
    transfrom->addChild(obj);
    transfrom->setValue(META_PROPERTY, coord);
    auto model = _database->getTilesModel();
    _database->push(new AddSceneObject(_database->getTilesModel(), model->index(isection.track.first, isection.track.second), obj));
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
