#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
#include "undo-redo.h"
#include "topology.h"
#include "ParentVisitor.h"
#include <QRegularExpression>

DatabaseManager::DatabaseManager(vsg::ref_ptr<vsg::Group> database, vsg::ref_ptr<vsg::Group> nodes, vsg::ref_ptr<vsg::Options> options)
  : root(nodes)
  , _database(database)
{
    builder = vsg::Builder::create();
    builder->options = options;

    topology = _database->getObject<route::Topology>(app::TOPOLOGY);
    if(!topology)
    {
        topology = route::Topology::create();
        _database->setObject(app::TOPOLOGY, topology);
    }

    auto modelroot = vsg::Group::create();
    modelroot->addChild(nodes);
    modelroot->addChild(database);

    vsg::visit<ParentIndexer>(modelroot);

    tilesModel = new SceneModel(modelroot, builder, undoStack);
}
DatabaseManager::~DatabaseManager()
{
}

void DatabaseManager::setUndoStack(QUndoStack *stack)
{
    undoStack = stack;
    tilesModel->setUndoStack(stack);
}

void DatabaseManager::setViewer(vsg::ref_ptr<vsg::Viewer> viewer)
{
    this->viewer = viewer;

    builder->options->setObject(app::VIEWER, viewer);

    vsg::StateInfo si;
    si.lighting = false;
    si.wireframe = true;
    vsg::GeometryInfo gi;
    _stdWireBox = builder->createBox(gi, si);

    builder->options->setObject(app::WIREFRAME, _stdWireBox.get());

    _stdAxis = vsg::Group::create();

    gi.dx = vsg::vec3(1.0f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    _stdAxis->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 1.0f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    _stdAxis->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 1.0f);
    gi.color = vsg::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    _stdAxis->addChild(builder->createBox(gi));
}

vsg::ref_ptr<vsg::Group> DatabaseManager::getDatabase() const noexcept { return _database; }

vsg::ref_ptr<vsg::Node> DatabaseManager::getStdWireBox()
{
    if(!_compiled)
        compile();

    return _stdWireBox;
}

vsg::ref_ptr<vsg::Node> DatabaseManager::getStdAxis()
{
    if(!_compiled)
        compile();

    return _stdAxis;
}

void DatabaseManager::writeTiles()
{
    auto removeBounds = [](vsg::VertexIndexDraw& object)
    {
        object.removeObject("bound");
    };
    LambdaVisitor<decltype (removeBounds), vsg::VertexIndexDraw> lv(removeBounds);

    auto removeParents = [](vsg::Node& node)
    {
        node.removeObject(app::PARENT);
    };
    LambdaVisitor<decltype (removeParents), vsg::Node> lvmp(removeParents);

    tilesModel->getRoot()->accept(lv);
    tilesModel->getRoot()->accept(lvmp);

    std::string path;
    if(!_database->getValue(app::PATH, path))
        throw DatabaseException(QObject::tr("Ошибка записи"));

    vsg::write(_database, path, builder->options);

    auto write = [options=builder->options](const auto node)
    {
        std::string path;
        if(!node->getValue(app::PATH, path))
            return;
        auto ext = vsg::lowerCaseFileExtension(path);
        if (ext == ".vsgt" || ext == ".vsgb")
        {
            vsg::VSG rw;
            rw.write(node, path, options);
        }
    };

    auto future = QtConcurrent::map(root->children.begin(), root->children.end(), write);
    future.waitForFinished();

    undoStack->setClean();
    vsg::visit<ParentIndexer>(tilesModel->getRoot());
}

void DatabaseManager::compile()
{
    Q_ASSERT(viewer);

    auto res = viewer->compileManager->compile(_stdAxis);
    vsg::updateViewer(*viewer, res);
    res = viewer->compileManager->compile(_stdWireBox);
    vsg::updateViewer(*viewer, res);

    _compiled = true;
}


