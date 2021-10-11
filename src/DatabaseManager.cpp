#include "databasemanager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>


DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, QObject *parent) : QObject(parent)
  , database(vsg::Group::create())
  , undoStack(stack)
{
    QFileInfo directory(path);
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

SceneModel *DatabaseManager::loadTiles()
{
    auto root = vsg::Group::create();
    TilesVisitor visitor(root);

    database->accept(visitor);
    tileFiles.subtract(visitor.filenames);

    QStringList filesToLoad(tileFiles.values());
    QFuture<vsg::ref_ptr<vsg::Group>> future = QtConcurrent::mappedReduced(filesToLoad, &DatabaseManager::read, addToGroup, root, QtConcurrent::OrderedReduce);
    future.waitForFinished();

    return new SceneModel(root);
}
void write(const vsg::ref_ptr<vsg::Node> node)
{
    std::string file;
    if(node->getValue(META_NAME, file))
        if(!vsg::write(node, file))
            throw (DatabaseException(file.c_str()));
}

void DatabaseManager::writeTiles(vsg::ref_ptr<vsg::Group> tiles)
{
    try {
        QFuture<void> future = QtConcurrent::map(tiles->children.begin(), tiles->children.end(), write);
        future.waitForFinished();
        undoStack->setClean();
    }  catch (DatabaseException &ex) {

    }
}


