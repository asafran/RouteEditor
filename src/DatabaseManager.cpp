#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
#include "TilesVisitor.h"
#include "undo-redo.h"
#include "topology.h"

DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QFileSystemModel *model, QObject *parent) : QObject(parent)
  , databasePath(path.toStdString())
  , builder(in_builder)
  , fsmodel(model)
  , modelsDir(vsg::getEnvPaths("RRS2_ROOT").begin()->c_str())
  , undoStack(stack)
{
    database = vsg::read_cast<vsg::Group>(databasePath, builder->options);
    if (!database)
        throw (DatabaseException(path));

    try {
        topology = database->children.at(TOPOLOGY_CHILD).cast<Topology>();
    }  catch (std::out_of_range) {
        topology = Topology::create();
        database->addChild(topology);
    }
    builder->options->objectCache->add(topology, TOPOLOGY_KEY);

    QFileInfo directory(path);
    QStringList filter("*subtile.vsg*");
    QStringList tileFiles;
    QDirIterator it(directory.absolutePath(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        tileFiles << it.next();
    std::function<QPair<QString, vsg::ref_ptr<vsg::Node>>(const QString &tilepath)> load = [in_builder](const QString &tilepath)
    {
        QFileInfo info(tilepath);
        try {
            auto tile = vsg::read_cast<vsg::Node>(tilepath.toStdString(), in_builder->options);
            if (tile)
                return qMakePair(info.fileName(), tile);
            else
                throw (DatabaseException(tilepath));
        }  catch (std::out_of_range) {
            throw (DatabaseException (tilepath));
        }

    };
    std::function reduce = [](QMap<QString, vsg::ref_ptr<vsg::Node>> &map, const QPair<QString, vsg::ref_ptr<vsg::Node>> &node)
    {
        map.insert(node.first, node.second);
    };
    QFuture<QMap<QString, vsg::ref_ptr<vsg::Node>>> future = QtConcurrent::mappedReduced(tileFiles, load, reduce, QtConcurrent::UnorderedReduce);
    future.waitForFinished();

    LoadTiles lt(future.result());
    database->accept(lt);
    tilesModel = new SceneModel(lt.tiles, builder, stack, this);
}
DatabaseManager::~DatabaseManager()
{
}
/*
vsg::ref_ptr<vsg::Node> DatabaseManager::read(const QString &path) const
{
    auto tile = vsg::read_cast<vsg::Node>(path.toStdString(), builder->options);
    if (tile)
    {
        //tile->setValue(META_NAME, path.toStdString());
        return tile;
    } else
        throw (DatabaseException(path));
}
*/
void DatabaseManager::addObject(vsg::dvec3 position, const QModelIndex &index) noexcept
{

    if(!loaded || (loadToSelected && !activeGroup.isValid()) || (!loadToSelected && !index.isValid()) || loaded.type == TrkObj)
        return;

    QModelIndex readindex = loadToSelected ? activeGroup : index;

    vsg::ref_ptr<SceneObject> obj;
    auto norm = vsg::normalize(position);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(static_cast<vsg::Node*>(readindex.internalPointer())->is_compatible(typeid (SceneObject)))
        position = vsg::dvec3();
    if (loaded.type == Trk)
    {
        if(auto trj = createTrajectory(loaded); trj != nullptr)
            obj = SceneTrajectory::create(trj, position, quat);
        else
            return;
    }
    else if(placeLoader)
        obj = SingleLoader::create(loaded.node, modelsDir.relativeFilePath(loaded.path).toStdString(), position, quat);
    else
        obj = SceneObject::create(loaded.node, position, quat);
    undoStack->push(new AddNode(tilesModel, readindex, obj));
}

void DatabaseManager::addTrack(SceneTrajectory *traj, double position) noexcept
{
    switch (loaded.type) {
    case Obj:
    {
        return;
    }
    case Trk:
    {
        if(position == 0.0)
            undoStack->push(new AddTrack(traj->traj, loaded));
        else
            createTrajectory(loaded, traj->traj);
        break;
    }
    case TrkObj:
    {
        break;
    }
    }
}

Trajectory *DatabaseManager::createTrajectory(const Loaded &loaded, Trajectory *prev)
{
    bool ok;
    QString name = QInputDialog::getText(qobject_cast<QWidget*>(parent()) , tr("Имя новой траектории"),
                                         tr("Имя:"), QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty())
    {
        while (topology->trajectories.find(name.toStdString()) != topology->trajectories.end())
        {
            name = QInputDialog::getText(qobject_cast<QWidget*>(parent()) , tr("Имя новой траектории"),
                                                 tr("Уже использовано, введите другое:"), QLineEdit::Normal, "", &ok);
            if(!ok)
                return nullptr;
        }
        undoStack->beginMacro(tr("Добавлена траектория %1").arg(name));
        auto cmd = new AddTrajectory(topology, name.toStdString(), prev);
        undoStack->push(cmd);
        undoStack->push(new AddTrack(cmd->_traj, loaded));
        undoStack->endMacro();
        return cmd->_traj;
    }
    return nullptr;
}

void DatabaseManager::activeGroupChanged(const QModelIndex &index) noexcept
{
    activeGroup = index;
}

void DatabaseManager::loaderButton(bool checked) noexcept
{
    placeLoader = checked;
}

void DatabaseManager::activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept
{
    auto path = fsmodel->filePath(selected.indexes().front());
    auto node = vsg::read_cast<vsg::Node>(path.toStdString(), builder->options);
    if(!node)
    {
        emit sendStatusText(tr("Ошибка при загрузке модели %1").arg(path), 5);
        loaded.node = nullptr;
        return;
    }
    builder->compile(node);
    loaded.type = Obj;
    if(node->getObject(META_TRACK))
        loaded.type = Trk;
    QDir dir(qgetenv("RRS2_ROOT"));
    loaded.path = dir.relativeFilePath(path);
    loaded.node = node;
}

void write(const vsg::ref_ptr<vsg::Node> node)
{
    std::string file;
    if(node->getValue(TILE_PATH, file))
    {
        node->removeObject(TILE_PATH);
        if(!vsg::write(node, file))
            throw (DatabaseException(file.c_str()));
        node->setValue(TILE_PATH, file);
    }
}

void DatabaseManager::writeTiles() noexcept
{
    auto removeBounds = [](vsg::VertexIndexDraw& object)
    {
        object.removeObject("bound");
    };
    LambdaVisitor<decltype (removeBounds), vsg::VertexIndexDraw> lv(removeBounds);
    tilesModel->getRoot()->accept(lv);
    vsg::write(database, databasePath, builder->options);
    try {
        QFuture<void> future = QtConcurrent::map(tilesModel->getRoot()->children, write);
        future.waitForFinished();
        undoStack->setClean();
    }  catch (DatabaseException &ex) {

    }
}


