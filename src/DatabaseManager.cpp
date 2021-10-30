#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>
#include "TilesVisitor.h"
#include "undo-redo.h"

DatabaseManager::DatabaseManager(const QString &path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Options> in_options, QFileSystemModel *model, QObject *parent) : QObject(parent)
  , database(vsg::Group::create())
  , options(in_options)
  , builder(in_builder)
  , fsmodel(model)
  , undoStack(stack)
{
    QFileInfo directory(path);
    fsWatcher = new QFileSystemWatcher(QStringList(directory.absolutePath() + QDir::separator() + "Tiles"), this);

    database = read(path);
    LoadTiles lt;
    lt.options = options;
    database->accept(lt);
    tilesModel = new SceneModel(lt.tiles, stack, this);
}
DatabaseManager::~DatabaseManager()
{

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

void DatabaseManager::addObject(const vsg::LineSegmentIntersector::Intersection &isection, const QModelIndex &index) noexcept
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
    auto group = static_cast<vsg::Node*>(readindex.internalPointer())->cast<vsg::Group>();
    if(group == nullptr)
        return;
    auto obj = SceneObject::create(activeFile.second, isection.localToWord, activeFile.first, vsg::translate(isection.localIntersection));
    tilesModel->addNode(readindex, new AddNode(group, obj));
}
void DatabaseManager::activeGroupChanged(const QModelIndex &index)
{
    activeGroup = index;
}

void DatabaseManager::activeFileChanged(const QItemSelection &selected, const QItemSelection &)
{
    std::string path = fsmodel->filePath(selected.indexes().front()).toStdString();
    auto node = vsg::read_cast<vsg::Node>(path, options);
    if(node)
        builder->compile(node);
    activeFile = std::make_pair(path, node);
}

void write(const vsg::ref_ptr<vsg::Node> node)
{
    std::string file;
    if(node->getValue(META_NAME, file))
        if(!vsg::write(node, file))
            throw (DatabaseException(file.c_str()));
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


