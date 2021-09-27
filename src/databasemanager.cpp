#include "databasemanager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>


DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, QObject *parent) : QObject(parent)
  , loadedTiles(vsg::Group::create())
  , cachedTiles(vsg::Group::create())
  , database(vsg::Group::create())
  , undoStack(stack)
{
    QFileInfo directory(path);
    fileTilesModel = new SceneModel(loadedTiles, this);
    cachedTilesModel =  new SceneModel(cachedTiles, undoStack, this);
    fsWatcher = new QFileSystemWatcher(QStringList(directory.absolutePath() + QDir::separator() + "Tiles"), this);

    QStringList filter("database_L5*");
    QDirIterator it(directory.absolutePath(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        tileFiles << it.next();

    database = read(path);
}
void addToGroup(vsg::ref_ptr<vsg::Group> &group, vsg::Node *node)
{
    group->addChild(vsg::ref_ptr<vsg::Node>(node));
}

vsg::Node* DatabaseManager::read(const QString &path)
{
    auto tile = vsg::read_cast<vsg::Node>(path.toStdString());
    if (tile)
    {
        tile->setValue(META_NAME, path.toStdString());
        return tile.release();
    } else
        throw (DatabaseException(path));
}

void DatabaseManager::loadTiles()
{
    try {
        loadedTiles->children.erase(loadedTiles->children.begin(), loadedTiles->children.end());
        QFuture<vsg::ref_ptr<vsg::Group>> future = QtConcurrent::mappedReduced(tileFiles, &DatabaseManager::read, addToGroup, loadedTiles, QtConcurrent::OrderedReduce);
        future.waitForFinished();

        emit updateViews();
    }  catch (DatabaseException &ex) {

    }

}
void DatabaseManager::writeTiles()
{

}

void DatabaseManager::updateTileCache()
{
    cachedTiles->children.erase(cachedTiles->children.begin(), cachedTiles->children.end());
    TilesVisitor visitor(cachedTiles);
    database->accept(visitor);

    emit updateViews();
}


