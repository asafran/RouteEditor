#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
//#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
#include "undo-redo.h"
#include "topology.h"
#include <QRegularExpression>

#include <execution>

/*
vsg::ref_ptr<vsg::Switch> prepareTile(vsg::Group *tile, vsg::ref_ptr<vsg::Builder> builder, vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer)
{
    vsg::GeometryInfo info;

    info.dx.set(3.0f, 0.0f, 0.0f);
    info.dy.set(0.0f, 3.0f, 0.0f);
    info.dz.set(0.0f, 0.0f, 3.0f);

    auto sphere = builder->createSphere(info);
    builder->compile(sphere);

    auto sw = vsg::Switch::create();

    for (auto& node : tile->children)
    {
        auto transform = node.cast<vsg::MatrixTransform>();
        auto addPoint = [transform, sphere, copyBuffer](vsg::VertexIndexDraw& vid)
        {
            auto bufferInfo = vid.arrays.front();
            auto vertarray = bufferInfo->data.cast<vsg::vec3Array>();
            for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
            {
                auto point = route::TerrainPoint::create(copyBuffer, bufferInfo, sphere, it);
                transform->addChild(point);
            }
        };
        LambdaVisitor<decltype (addPoint), vsg::VertexIndexDraw> lv(addPoint);
        transform->accept(lv);
        sw->addChild(route::Tiles, transform);
    }

    return sw;
}
*/
DatabaseManager::DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QObject *parent) : QObject(parent)
  , _root(vsg::Group::create())
  , _databasePath(path)
  , _builder(in_builder)
  , _undoStack(stack)
{
    _database = vsg::read_cast<vsg::Group>(_databasePath.toStdString(), _builder->options);
    if (!_database)
        throw (DatabaseException(path));
    try {
        _topology = _database->children.at(TOPOLOGY_CHILD).cast<route::Topology>();
    }  catch (std::out_of_range) {
        _topology = route::Topology::create();
        _database->addChild(_topology);
    }
    _builder->options->objectCache->add(_topology, TOPOLOGY_KEY);
}
DatabaseManager::~DatabaseManager()
{
}

SceneModel *DatabaseManager::loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, double tileLOD, double pointsLOD, float size)
{
    QFileInfo directory(_databasePath);
    QStringList filter("*L*_X*_Y*_subtile.vsg*");
    QStringList tileFiles;
    QDirIterator it(directory.absolutePath(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    std::vector<int> levels;
    while (it.hasNext())
    {
        auto path = it.next();
        tileFiles << path;
        auto index = path.lastIndexOf(QRegularExpression("L[0-9]"));
        levels.push_back(path.at(++index).toLatin1() - '0');
    }
    auto level = *std::max_element(levels.begin(), levels.end());
    tileFiles = tileFiles.filter(QRegularExpression(QString(".+L%1_X[0-9]+_Y[0-9]+_subtile.vsg.").arg(level)));

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    info.dx.set(size, 0.0f, 0.0f);
    info.dy.set(0.0f, size, 0.0f);
    info.dz.set(0.0f, 0.0f, size);

    state.lighting = false;

    auto sphere = _builder->createSphere(info, state);

    auto tiles = vsg::Group::create();
    auto scene = vsg::Group::create();

    std::mutex m;
    auto load = [sphere, copyBuffer, pointsLOD, size, tiles, scene, tileLOD, this, &m](const QString &tilepath)
    {
        auto file = vsg::read_cast<vsg::Node>(tilepath.toStdString(), _builder->options);
        if (file)
        {
            vsg::ref_ptr<vsg::Switch> tile(file->cast<vsg::Switch>());

            if(!tile)
            {
                if(auto group = file->cast<vsg::Group>(); group)
                {
                    tile = vsg::Switch::create();
                    for (auto& node : group->children)
                    {
                        auto transform = node.cast<vsg::MatrixTransform>();
                        tile->addChild(route::Tiles, transform);
                    }
                }
                else
                {
                    std::scoped_lock lock(m);
                    emit sendStatusText(tr("Ошибка чтения БД, файл: %1").arg(tilepath), 1000);
                    return;
                }
            }

            auto pointGroup = vsg::Group::create();
            tile->addChild(route::Points, pointGroup);

            auto traverseTiles = [pointGroup, sphere, copyBuffer, pointsLOD, radius=size/2](vsg::MatrixTransform &transform)
            {
                auto addPoint = [=](vsg::VertexIndexDraw& vid)
                {
                    auto bufferInfo = vid.arrays.front();
                    auto vertarray = bufferInfo->data.cast<vsg::vec3Array>();
                    for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
                    {
                        auto point = route::TerrainPoint::create(copyBuffer, bufferInfo, transform.matrix, sphere, it);

                        auto lod = vsg::LOD::create();
                        vsg::LOD::Child hires{pointsLOD, point};
                        vsg::LOD::Child dummy{0.0, vsg::Node::create()};
                        lod->addChild(hires);
                        lod->addChild(dummy);

                        vsg::dsphere bound;
                        bound.center = transform.matrix * vsg::dvec3(*it);
                        bound.radius = radius;
                        lod->bound = bound;

                        pointGroup->addChild(lod);
                    }
                };
                LambdaVisitor<decltype (addPoint), vsg::VertexIndexDraw> lv(addPoint);
                transform.accept(lv);
            };
            LambdaVisitor<decltype (traverseTiles), vsg::MatrixTransform> lv(traverseTiles);
            lv.traversalMask = route::Tiles;
            tile->accept(lv);

            auto sceneLOD = vsg::LOD::create();
            vsg::LOD::Child hires{tileLOD, tile};
            vsg::LOD::Child dummy{0.0, vsg::Node::create()};
            sceneLOD->addChild(hires);
            sceneLOD->addChild(dummy);

            vsg::ComputeBounds cb;
            cb.traversalMask = route::SceneObjects | route::Tiles;
            tile->accept(cb);
            vsg::dsphere bound;
            bound.center = (cb.bounds.min + cb.bounds.max) * 0.5;
            bound.radius = length(cb.bounds.max - cb.bounds.min) * 0.5;
            sceneLOD->bound = bound;

            {
                std::scoped_lock lock(m);
                tiles->addChild(tile);
                scene->addChild(sceneLOD);
                _files.insert(std::pair{tile.get(), tilepath});
            }
        }
    };
    std::for_each(std::execution::par, tileFiles.begin(), tileFiles.end(), load);

    _root->addChild(scene);

    _tilesModel = new SceneModel(tiles, _builder, _undoStack, this);
    return _tilesModel;
}

/*
void DatabaseManager::addTrack(const vsg::dvec3 &pos) noexcept
{
    auto sphere = builder->createSphere();
    auto sleeper = builder->createBox();

    builder->compile(sphere);
    builder->compile(sleeper);

    std::vector<vsg::ref_ptr<route::SplinePoint>> points;
    points.push_back(route::SplinePoint::create(pos - vsg::dvec3(10.0, 0.0, 0.0), sphere));
    points.push_back(route::SplinePoint::create(pos, sphere));
    points.push_back(route::SplinePoint::create(pos + vsg::dvec3(100000.0, 0.0, 0.0), sphere));
    points.push_back(route::SplinePoint::create(pos + vsg::dvec3(200000.0, 0.0, 0.0), sphere));
    std::vector<vsg::vec3> geometry;
    geometry.emplace_back(0.0f, 0.0f, 0.0f);
    geometry.emplace_back(0.0f, 2.0f, 0.0f);
    geometry.emplace_back(2.0f, 2.0f, 0.0f);
    geometry.emplace_back(2.0f, 0.0f, 0.0f);
    auto trajectory = route::SplineTrajectory::create("name", builder, points, geometry, sleeper, 1.0);
    root->addChild(trajectory);
}*/


void DatabaseManager::writeTiles() noexcept
{
    auto removeBounds = [](vsg::VertexIndexDraw& object)
    {
        object.removeObject("bound");
    };
    LambdaVisitor<decltype (removeBounds), vsg::VertexIndexDraw> lv(removeBounds);
    auto removePoints = [](vsg::Switch& sw)
    {
        for (auto it = sw.children.begin(); it != sw.children.end(); ++it)
        {
            if (it->mask == route::Points)
            {
                sw.children.erase(it);
                break;
            }
        }
    };
    LambdaVisitor<decltype (removePoints), vsg::Switch> lvp(removePoints);
    _tilesModel->getRoot()->accept(lvp);

    vsg::write(_database, _databasePath.toStdString(), _builder->options);

    auto write = [options=_builder->options](const auto node)
    {
        auto filename = node.second.toStdString();
        auto ext = vsg::lowerCaseFileExtension(filename);
        if (ext == ".vsgt" || ext == ".vsgb")
        {
            vsg::VSG rw;
            rw.write(node.first, filename, options);
        }
    };

    std::for_each(std::execution::par, _files.begin(), _files.end(), write);
    _undoStack->setClean();
}


