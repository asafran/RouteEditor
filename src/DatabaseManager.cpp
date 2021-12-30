#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
//#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
//#include "TilesVisitor.h"
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
DatabaseManager::DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QFileSystemModel *model, QObject *parent) : QObject(parent)
  , root(vsg::Group::create())
  , databasePath(path)
  , builder(in_builder)
  , fsmodel(model)
  , modelsDir(vsg::getEnvPaths("RRS2_ROOT").begin()->c_str())
  , undoStack(stack)
{
    database = vsg::read_cast<vsg::Group>(databasePath.toStdString(), builder->options);
    if (!database)
        throw (DatabaseException(path));
    try {
        topology = database->children.at(TOPOLOGY_CHILD).cast<route::Topology>();
    }  catch (std::out_of_range) {
        topology = route::Topology::create();
        database->addChild(topology);
    }
    builder->options->objectCache->add(topology, TOPOLOGY_KEY);
}
DatabaseManager::~DatabaseManager()
{
}

SceneModel *DatabaseManager::loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, double tileLOD, double pointsLOD, float size)
{
    QFileInfo directory(databasePath);
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

    auto prepare = [builder=builder, copyBuffer, pointsLOD, size](vsg::Group *tile)
    {
        vsg::GeometryInfo info;
        vsg::StateInfo state;

        info.dx.set(size, 0.0f, 0.0f);
        info.dy.set(0.0f, size, 0.0f);
        info.dz.set(0.0f, 0.0f, size);

        state.lighting = false;

        auto sphere = builder->createSphere(info, state);
        builder->compile(sphere);

        auto sw = vsg::Switch::create();

        for (auto& node : tile->children)
        {
            auto transform = node.cast<vsg::MatrixTransform>();
            auto pointSwitch = vsg::Switch::create();
            auto pointGroup = vsg::Group::create();
            pointSwitch->addChild(route::Points, pointGroup);

            auto addPoint = [pointGroup, sphere, copyBuffer, pointsLOD, size=size/2](vsg::VertexIndexDraw& vid)
            {
                auto bufferInfo = vid.arrays.front();
                auto vertarray = bufferInfo->data.cast<vsg::vec3Array>();
                for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
                {
                    auto point = route::TerrainPoint::create(copyBuffer, bufferInfo, sphere, it);

                    auto lod = vsg::LOD::create();
                    vsg::LOD::Child hires{pointsLOD, point};
                    vsg::LOD::Child dummy{0.0, vsg::Node::create()};
                    lod->addChild(hires);
                    lod->addChild(dummy);

                    vsg::dsphere bound;
                    bound.center = *it;
                    bound.radius = size;
                    lod->bound = bound;

                    pointGroup->addChild(lod);
                }
            };
            LambdaVisitor<decltype (addPoint), vsg::VertexIndexDraw> lv(addPoint);
            transform->accept(lv);
            transform->addChild(pointSwitch);
            sw->addChild(route::Tiles, transform);
        }

        return sw;
    };

    auto tiles = vsg::Group::create();
    auto scene = vsg::Group::create();

    std::mutex m;
    auto load = [prepare, tiles, scene, tileLOD, this, &m](const QString &tilepath)
    {
        auto file = vsg::read_cast<vsg::Node>(tilepath.toStdString(), builder->options);
        if (file)
        {
            vsg::ref_ptr<vsg::Node> tile;

            if(file->is_compatible( typeid (vsg::Switch)))
                tile = file;
            else if(auto group = file->cast<vsg::Group>(); group)
                tile = prepare(group);
            else
            {
                std::scoped_lock lock(m);
                emit sendStatusText(tr("Ошибка чтения БД, файл: %1").arg(tilepath), 1000);
                return;
            }

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
                files.insert(std::pair{tile.get(), tilepath});
            }
        }
    };
    std::for_each(std::execution::par, tileFiles.begin(), tileFiles.end(), load);

    root->addChild(scene);

    tilesModel = new SceneModel(tiles, builder, undoStack, this);
    return tilesModel;
}


void DatabaseManager::addToClicked(const FindNode &found) noexcept
{
    /*
    if(found.track.first != nullptr)
    {

    } else if(found.)

    auto index = tilesModel->index(tile.first, tile.second);
    bool canAddToIntersected = !loadToSelected && index.isValid();
    bool canAddToSelected = loadToSelected && activeGroup.isValid();
    if(!loaded || (!canAddToIntersected && !canAddToSelected))
        return;

    QModelIndex readindex = loadToSelected ? activeGroup : index;

    vsg::ref_ptr<route::SceneObject> obj;
    auto norm = vsg::normalize(local);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(placeLoader)
        obj = route::SingleLoader::create(loaded, loadedPath, local, quat);
    else
        obj = route::SceneObject::create(loaded, local, quat);
    undoStack->push(new AddSceneObject(tilesModel, readindex, obj));
    */
}

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
    loaded = vsg::read_cast<vsg::Node>(path.toStdString(), builder->options);
    loadedPath = modelsDir.relativeFilePath(path);

    vsg::Objects;
}

void DatabaseManager::writeTiles() noexcept
{
    auto removeBounds = [](vsg::VertexIndexDraw& object)
    {
        object.removeObject("bound");
    };
    LambdaVisitor<decltype (removeBounds), vsg::VertexIndexDraw> lv(removeBounds);
    tilesModel->getRoot()->accept(lv);
    vsg::write(database, databasePath.toStdString(), builder->options);

    auto write = [options=builder->options](const auto node)
    {
        auto filename = node.second.toStdString();
        auto ext = vsg::lowerCaseFileExtension(filename);
        if (ext == ".vsgt" || ext == ".vsgb")
        {
            vsg::VSG rw;
            rw.write(node.first, filename, options);
        }
    };

    std::for_each(std::execution::par, files.begin(), files.end(), write);
    undoStack->setClean();
}


