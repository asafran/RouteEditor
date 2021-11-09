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
    fsWatcher = new QFileSystemWatcher(QStringList(directory.absolutePath() + QDir::separator() + "Tiles"), this);

    database = read(path);
    LoadTiles lt;
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
        return tile.release_nodelete();
    } else
        throw (DatabaseException(path));
}

void DatabaseManager::addObject(const vsg::dvec3& position, const QModelIndex &index) noexcept
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
    vsg::ref_ptr<SceneObject> obj;

    auto norm = vsg::normalize(position);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);
    if(placeLoader)
        obj = SingleLoader::create(activeFile.second, modelsDir.relativeFilePath(activeFile.first).toStdString(), vsg::translate(position) * vsg::rotate(quat), quat);
    else
        obj = SceneObject::create(activeFile.second, vsg::translate(position) * vsg::rotate(quat), quat);
    tilesModel->addNode(readindex, new AddNode(group, obj));
}
void DatabaseManager::activeGroupChanged(const QModelIndex &index) noexcept
{
    activeGroup = index;
}

void DatabaseManager::loaderAction(bool checked) noexcept
{
    placeLoader = checked;
}

void DatabaseManager::activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept
{
    auto path = fsmodel->filePath(selected.indexes().front());
    auto node = vsg::read_cast<vsg::Node>(path.toStdString());
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


