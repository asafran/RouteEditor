#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include <execution>
#include <QInputDialog>
#include <vsg/app/Viewer.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/io/write.h>
#include "undo-redo.h"
#include "topology.h"
#include <QRegularExpression>

DatabaseManager::DatabaseManager(vsg::ref_ptr<route::Route> in_route, vsg::ref_ptr<vsg::Options> options)
  : root(vsg::Group::create())
  , route(in_route)
{
    builder = vsg::Builder::create();
    builder->options = options;

    root->addChild(route);

    auto fixPaths = [](vsg::PagedLOD& plod)
    {
        QFileInfo fi(plod.filename.c_str());
        plod.filename = fi.fileName().toStdString();
    };
    LambdaVisitor<decltype (fixPaths), vsg::PagedLOD> fp(fixPaths);
    in_route->plods->accept(fp);

    tilesModel = new SceneModel(route, builder);
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

    builder->options->setObject(app::WIREFRAME, _stdWireBox);

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
    tilesModel->getRoot()->accept(lv);

    vsg::visit<route::SetStatic>(root);

    std::string path;
    if(!route->getValue(app::PATH, path))
        throw DatabaseException(QObject::tr("Ошибка записи"));

    vsg::write(route, path, builder->options);

    auto write = [options=builder->options](const auto node)
    {
        std::string path;
        if(!node->getValue(app::PATH, path))
            return;
        vsg::write(node, path, options);
    };

    std::for_each(std::execution::par, route->tiles->childrenObjects().begin(), route->tiles->childrenObjects().end(), write);

    undoStack->setClean();
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


