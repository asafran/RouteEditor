#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>
#include "TilesVisitor.h"
#include "undo-redo.h"

DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QFileSystemModel *model, QObject *parent) : QObject(parent)
  , database(vsg::Group::create())
  , builder(in_builder)
  , fsmodel(model)
  , modelsDir(vsg::getEnvPaths("RRS2_ROOT").begin()->c_str())
  , undoStack(stack)
{
    QFileInfo directory(path);
    QStringList filter("*subtile.vsgt");
    QStringList tileFiles;
    QDirIterator it(directory.absolutePath(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        tileFiles << it.next();
    std::function<QPair<QString, vsg::ref_ptr<vsg::Node>>(const QString &path)> load = [this](const QString &path)
    {
        QFileInfo info(path);
        return qMakePair(info.fileName(), read(path));
    };
    std::function reduce = [](QMap<QString, vsg::ref_ptr<vsg::Node>> &map, const QPair<QString, vsg::ref_ptr<vsg::Node>> &node)
    {
        map.insert(node.first, node.second);
    };
    QFuture<QMap<QString, vsg::ref_ptr<vsg::Node>>> future = QtConcurrent::mappedReduced(tileFiles, load, reduce, QtConcurrent::UnorderedReduce);
    future.waitForFinished();

    database = read(path);
    LoadTiles lt(future.result());
    database->accept(lt);
    tilesModel = new SceneModel(lt.tiles, builder, stack, this);
}
DatabaseManager::~DatabaseManager()
{
}

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

void DatabaseManager::addObject(vsg::dvec3 position, const QModelIndex &index) noexcept
{
    QModelIndex readindex;
    if(!activeFile.second)
        return;
    if(activeGroup.isValid())
        readindex = activeGroup;
    else if (index.isValid())
        readindex = index;
    else
        return;
    vsg::ref_ptr<SceneObject> obj;
    auto norm = vsg::normalize(position);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(static_cast<vsg::Node*>(readindex.internalPointer())->is_compatible(typeid (SceneObject)))
        position = vsg::dvec3();

    if(placeLoader)
        obj = SingleLoader::create(activeFile.second, modelsDir.relativeFilePath(activeFile.first).toStdString(), vsg::translate(position), quat);
    else
        obj = SceneObject::create(activeFile.second, vsg::translate(position), quat);
    undoStack->push(new AddNode(tilesModel, readindex, obj));
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
    auto path = fsmodel->fileInfo(selected.indexes().front());
    auto node = vsg::read_cast<vsg::Node>(path.canonicalFilePath().toStdString(), builder->options);
    if(node)
        builder->compile(node);
    activeFile = std::make_pair(path.canonicalFilePath(), node);
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
    try {
        QFuture<void> future = QtConcurrent::map(tilesModel->getRoot()->children, write);
        future.waitForFinished();
        undoStack->setClean();
    }  catch (DatabaseException &ex) {

    }
}


