#include "DatabaseManager.h"
#include "LambdaVisitor.h"
#include "vsgGIS/TileDatabase.h"
//#include <QtConcurrent/QtConcurrent>
#include <QInputDialog>
#include "undo-redo.h"
#include "topology.h"
#include <QRegularExpression>

#include <execution>

DatabaseManager::DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> builder, QObject *parent) : QObject(parent)
  , _root(vsg::Group::create())
  , _databasePath(path)
  , _builder(builder)
  //, _compiler(compiler)
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

void DatabaseManager::addPoints(const vsg::Node *tile, vsg::ref_ptr<vsg::Node> sphere, vsg::ref_ptr<vsg::Group> points)
{
    auto traverseTiles = [sphere, points, copyBuffer=_copyBufferCmd](const vsg::MatrixTransform &transform)
    {
        auto addPoint = [=](const vsg::VertexIndexDraw& vid)
        {
            auto bufferInfo = vid.arrays.front();
            auto vertarray = bufferInfo->data.cast<vsg::vec3Array>();
            for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
            {
                points->addChild(route::TerrainPoint::create(copyBuffer, bufferInfo, transform.matrix, sphere, it));
            }
        };
        CLambdaVisitor<decltype (addPoint), vsg::VertexIndexDraw> lv(addPoint);
        transform.accept(lv);
    };
    CLambdaVisitor<decltype (traverseTiles), vsg::MatrixTransform> lv(traverseTiles);
    lv.traversalMask = route::Tiles;
    tile->accept(lv);
}

SceneModel *DatabaseManager::loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, vsg::ref_ptr<vsg::CopyAndReleaseImage> copyImage)
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
    auto lodp = settings.value("LOD_POINTS", 0.1).toDouble();

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    info.dx.set(size, 0.0f, 0.0f);
    info.dy.set(0.0f, size, 0.0f);
    info.dz.set(0.0f, 0.0f, size);

    state.lighting = false;

    auto sphere = _builder->createSphere(info, state);

    std::mutex m;
    auto load = [tiles, scene, lodt, this, &m, lodp, sphere](const QString &tilepath)
    {
        auto file = vsg::read_cast<vsg::Node>(tilepath.toStdString(), _builder->options);
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

    _root->addChild(scene);

    _tilesModel = new SceneModel(tiles, _builder, _undoStack, this);



    return _tilesModel;
}

void DatabaseManager::writeTiles() noexcept
{
    auto removeBounds = [](vsg::VertexIndexDraw& object)
    {
        object.removeObject("bound");
    };
    LambdaVisitor<decltype (removeBounds), vsg::VertexIndexDraw> lv(removeBounds);

    auto removePoints = [](vsg::Switch& sw)
    {
        auto points = std::find_if(sw.children.begin(), sw.children.end(), [](const vsg::Switch::Child &ch)
        {
            return (ch.mask & route::Points) != 0;
        });
        sw.children.erase(points);
        /*
        for (auto it = sw.children.begin(); it != sw.children.end(); ++it)
        {
            if (it->mask == route::Points)
            {
                auto group = it->node.cast<vsg::Group>();
                group->children.clear();
                break;
            }
        }*/
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


