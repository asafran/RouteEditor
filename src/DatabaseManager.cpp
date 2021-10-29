#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>
#include "TilesVisitor.h"

DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, vsg::ref_ptr<vsg::Options> in_options, QObject *parent) : QObject(parent)
  , database(vsg::Group::create())
  , options(in_options)
  , undoStack(stack)
{
    QFileInfo directory(path);
    fsWatcher = new QFileSystemWatcher(QStringList(directory.absolutePath() + QDir::separator() + "Tiles"), this);
/*
    QStringList filter("database_L5*");
    QDirIterator it(directory.absolutePath(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
        tileFiles << it.next();
*/
    database = read(path);
    LoadTiles lt;
    lt.options = options;
    database->accept(lt);
    tilesModel = new SceneModel(lt.tiles, stack, this);
}
/*
void addToGroup(vsg::ref_ptr<vsg::Group> &group, vsg::Node *node)
{
    group->children.emplace_back(node);
}
*/
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
/*
SceneModel *DatabaseManager::loadTiles()
{
    auto root = vsg::Group::create();
    root->children = cachedTilesModel->getRoot()->children;
    tileFiles.subtract(culledFiles);

    QStringList filesToLoad(tileFiles.values());
    QFuture<vsg::ref_ptr<vsg::Group>> future = QtConcurrent::mappedReduced(filesToLoad, &DatabaseManager::read, addToGroup, root, QtConcurrent::OrderedReduce);
    future.waitForFinished();

    return new SceneModel(root);
}
void _check(vsg::ref_ptr<vsg::Group> root, vsg::ref_ptr<vsg::PagedLOD> plod)
{
    if(!culled.contains(plod->filename.c_str()))
        if(auto group = plod->children.front().node.cast<vsg::Group>(); group && group->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
        {
            group->setValue(META_NAME, plod->filename);
            root->addChild(group);
            culled.insert(QFileInfo(plod->filename.c_str()).canonicalFilePath());
        }
}

void _updateTileCache(vsg::ref_ptr<vsg::Group> root, QSet<QString> &culled, vsg::ref_ptr<vsg::DatabasePager> database)
{
    for (uint32_t index = database->pagedLODContainer->activeList.head; index != 0;)
    {
        auto& element = database->pagedLODContainer->elements[index];

        _check(root, element.plod, culled);

        index = element.next;
    }
    for (uint32_t index = database->pagedLODContainer->inactiveList.head; index != 0;)
    {
        auto& element = database->pagedLODContainer->elements[index];

        _check(root, element.plod, culled);

        index = element.next;
    }

}

void DatabaseManager::updateTileCache()
{
    auto avCount = pager->pagedLODContainer->availableList.count;
    if(avCount < prevAvCount)
    {
        prevAvCount = avCount;
        cachedTilesModel->fetchMore(QModelIndex(), _updateTileCache, culledFiles, pager);
    }
    else if(avCount > prevAvCount)
    {
        prevAvCount = avCount;
        cachedTilesModel->getRoot()->children.clear();
        culledFiles.clear();
        cachedTilesModel->fetchMore(QModelIndex(), _updateTileCache, culledFiles, pager);
    }
}
*/
void write(const vsg::ref_ptr<vsg::Node> node)
{
    std::string file;
    if(node->getValue(META_NAME, file))
        if(!vsg::write(node, file))
            throw (DatabaseException(file.c_str()));
}

void DatabaseManager::writeTiles()
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


