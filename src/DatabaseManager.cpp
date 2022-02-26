#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
//#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
#include "undo-redo.h"
#include "topology.h"
#include "ParentVisitor.h"
#include <QRegularExpression>

#include <execution>

DatabaseManager::DatabaseManager(QString path, vsg::ref_ptr<vsg::Builder> in_builder, QObject *parent) : QObject(parent)
  , root(vsg::Group::create())
  , builder(in_builder)
  , _databasePath(path)
  //, _builder(builder)
  //, _compiler(compiler)
  //, _undoStack(stack)
{
    _database = vsg::read_cast<vsg::Group>(_databasePath.toStdString(), builder->options);
    if (!_database)
        throw (DatabaseException(path));

        topology = _database->getObject<route::Topology>(app::TOPOLOGY);
        if(!topology)
        {
            topology = route::Topology::create();
            _database->setObject(app::TOPOLOGY, topology);
        }
        //topology->assignBuilder(builder);

    builder->options->objectCache->add(topology, app::TOPOLOGY);

    vsg::StateInfo si;
    si.lighting = false;
    si.wireframe = true;
    vsg::GeometryInfo gi;
    stdWireBox = builder->createBox(gi, si);

    stdAxis = vsg::Group::create();

    gi.dx = vsg::vec3(1.0f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    stdAxis->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 1.0f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 0.1f);
    gi.color = vsg::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    stdAxis->addChild(builder->createBox(gi));
    gi.dx = vsg::vec3(0.1f, 0.0f, 0.0f);
    gi.dy = vsg::vec3(0.0f, 0.1f, 0.0f);
    gi.dz = vsg::vec3(0.0f, 0.0f, 1.0f);
    gi.color = vsg::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    stdAxis->addChild(builder->createBox(gi));

    root->addChild(stdWireBox); //for compilation
    root->addChild(stdAxis);
    root->addChild(topology);
}
DatabaseManager::~DatabaseManager()
{
}

vsg::ref_ptr<vsg::Group> DatabaseManager::getDatabase() const noexcept { return _database; }

void DatabaseManager::addPoints(const vsg::Node *tile, vsg::ref_ptr<vsg::Node> sphere, vsg::ref_ptr<vsg::Group> points)
{
    auto traverseTiles = [sphere, points, copyBuffer=_copyBufferCmd, box=stdWireBox](const vsg::MatrixTransform &transform)
    {
        auto addPoint = [=](const vsg::VertexIndexDraw& vid)
        {
            auto bufferInfo = vid.arrays.front();
            auto vertarray = bufferInfo->data.cast<vsg::vec3Array>();
            for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
            {
                auto point = route::TerrainPoint::create(copyBuffer, bufferInfo, transform.matrix, sphere, box, it);
                point->recalculateWireframe();
                points->addChild(point);
            }
        };
        CLambdaVisitor<decltype (addPoint), vsg::VertexIndexDraw> lv(addPoint);
        transform.accept(lv);
    };
    CLambdaVisitor<decltype (traverseTiles), vsg::MatrixTransform> lv(traverseTiles);
    lv.traversalMask = route::Tiles;
    tile->accept(lv);
}

void DatabaseManager::loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, vsg::ref_ptr<vsg::CopyAndReleaseImage> copyImage)
{
    _copyBufferCmd = copyBuffer;
    _copyImageCmd = copyImage;

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

    auto tiles = vsg::Group::create();
    auto scene = vsg::Group::create();

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    auto lodt = settings.value("LOD_TILES", 0.5).toDouble();

    auto size = settings.value("POINTSIZE", 3).toInt();
    auto lodp = settings.value("LOD_POINTS", 0.8).toDouble();

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    info.dx.set(size, 0.0f, 0.0f);
    info.dy.set(0.0f, size, 0.0f);
    info.dz.set(0.0f, 0.0f, size);

    state.lighting = false;

    auto sphere = builder->createSphere(info, state);

    std::mutex m;
    auto load = [tiles, scene, lodt, this, &m, lodp, sphere](const QString &tilepath)
    {
        auto file = vsg::read_cast<vsg::Node>(tilepath.toStdString(), builder->options);
        if (file)
        {
            vsg::ref_ptr<vsg::Switch> tile(file->cast<vsg::Switch>());
            vsg::ref_ptr<vsg::Group> pointsSW;

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
                    pointsSW = PointsGroup::create();
                    tile->addChild(route::Points, pointsSW);
                }
                else
                {
                    std::scoped_lock lock(m);
                    emit sendStatusText(tr("Ошибка чтения БД, файл: %1").arg(tilepath), 1000);
                    return;
                }
            } else
            {
                auto object = std::find_if(tile->children.begin(), tile->children.end(), [](const vsg::Switch::Child &ch)
                {
                    return (ch.mask & route::Points) != 0;
                });
                Q_ASSERT(object < tile->children.cend());
                pointsSW = object->node.cast<vsg::Group>();
            }

            auto sceneLOD = vsg::LOD::create();
            auto pointsLOD = vsg::LOD::create();

            vsg::LOD::Child hires{lodt, tile};
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

            auto pointsGroup = vsg::Group::create();

            addPoints(tile, sphere, pointsGroup);

            vsg::LOD::Child hiresp{lodp, pointsGroup};
            vsg::LOD::Child dummyp{0.0, vsg::Node::create()};
            pointsLOD->addChild(hiresp);
            pointsLOD->addChild(dummyp);

            pointsLOD->bound = bound;

            pointsSW->addChild(pointsLOD);

            {
                std::scoped_lock lock(m);
                tiles->addChild(tile);
                scene->addChild(sceneLOD);
                _files.insert(std::pair{tile.get(), tilepath});
            }
        }
    };
    std::for_each(std::execution::par, tileFiles.begin(), tileFiles.end(), load);

    root->addChild(scene);

    auto modelroot = vsg::Group::create();
    modelroot->addChild(tiles);
    modelroot->addChild(topology);

    vsg::visit<ParentIndexer>(modelroot);

    tilesModel = new SceneModel(modelroot, builder, undoStack, this);

    //return tilesModel;
}

vsg::ref_ptr<vsg::Node> DatabaseManager::getStdWireBox()
{
    return stdWireBox;
}

vsg::ref_ptr<vsg::Node> DatabaseManager::getStdAxis()
{
    return stdAxis;
}

void DatabaseManager::writeTiles() noexcept
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


    vsg::write(_database, _databasePath.toStdString(), builder->options);

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

    std::for_each(std::execution::par, _files.begin(), _files.end(), write);
    undoStack->setClean();

    vsg::visit<ParentIndexer>(tilesModel->getRoot());
}


