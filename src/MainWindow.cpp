#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <vsgQt/ViewerWindow.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
#include "AddDialog.h"
#include "undo-redo.h"
#include "LambdaVisitor.h"
#include "ContentManager.h"
#include "ObjectPropertiesEditor.h"


MainWindow::MainWindow(QString routePath, QString skybox, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pathDB(routePath)
{

    ui->setupUi(this);

    constructWidgets();

    undoStack = new QUndoStack(this);
    database = new DatabaseManager(pathDB, undoStack, builder, this);

    initializeTools();

    undoView = new QUndoView(undoStack, ui->tabWidget);
    ui->tabWidget->addTab(undoView, tr("Действия"));

    connect(ui->actionUndo, &QAction::triggered, undoStack, &QUndoStack::undo);
    connect(ui->actionRedo, &QAction::triggered, undoStack, &QUndoStack::redo);

    connect(ui->removeButt, &QPushButton::pressed, [this]()
    {
        if(ui->tilesView->selectionModel()->selectedIndexes().front().isValid())
        {
            auto selected = sorter->mapToSource(ui->tilesView->selectionModel()->selectedIndexes().front());
            undoStack->push(new RemoveNode(database->getTilesModel(), selected));
        }
    });

}


void MainWindow::initializeTools()
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    toolbox = new QToolBox(ui->splitter);
    ui->splitter->addWidget(toolbox);

    auto ope = ObjectPropertiesEditor::create(database, toolbox);
    toolbox->addItem(ope, tr("Выбрать и переместить объекты"));
    auto cm = new ContentManager(database, qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "objects", toolbox);
    toolbox->addItem(cm, tr("Добавить объект"));

    database->getRoot()->addChild(ope);
    ope->ref();

    connect(sorter, &TilesSorter::selectionChanged, ope, &ObjectPropertiesEditor::selectObject);
    connect(ope, &Tool::objectClicked, sorter, &TilesSorter::select);
    connect(ope, &Tool::deselect, ui->tilesView->selectionModel(), &QItemSelectionModel::clearSelection);
    connect(sorter, &TilesSorter::frontSelectionChanged, cm, &ContentManager::activeGroupChanged);
}

void MainWindow::intersection(const FindNode& isection)
{
    qobject_cast<Tool*>(toolbox->currentWidget())->intersection(isection);
}

QWindow* MainWindow::initilizeVSGwindow()
{
    auto options = vsg::Options::create();
    options->fileCache = vsg::getEnv("RRS2_CACHE");
    options->paths = vsg::getEnvPaths("RRS2_ROOT");

    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());
    options->objectCache = vsg::ObjectCache::create();

    vsg::RegisterWithObjectFactoryProxy<route::SceneObject>();
    vsg::RegisterWithObjectFactoryProxy<route::SingleLoader>();
    vsg::RegisterWithObjectFactoryProxy<route::SceneTrajectory>();
    vsg::RegisterWithObjectFactoryProxy<route::Topology>();
    vsg::RegisterWithObjectFactoryProxy<route::SplineTrajectory>();
    //vsg::RegisterWithObjectFactoryProxy<route::TerrainPoint>();

    builder = vsg::Builder::create();
    builder->options = options;

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = APPLICATION_NAME;

    /*
    SceneModel *scenemodel = new SceneModel(database->getRoot(), this);
    ui->sceneTreeView->setModel(scenemodel);
    ui->sceneTreeView->expandAll();
*/
    viewerWindow = new vsgQt::ViewerWindow();
    viewerWindow->traits = windowTraits;
    viewerWindow->viewer = vsg::Viewer::create();

    // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
    viewerWindow->initializeCallback = [&, this, options](vsgQt::ViewerWindow& vw, uint32_t width, uint32_t height) {

        auto& window = vw.windowAdapter;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) viewer = vsg::Viewer::create();

        viewer->addWindow(window);

        auto memoryBufferPools = vsg::MemoryBufferPools::create("Staging_MemoryBufferPool", vsg::ref_ptr<vsg::Device>(window->getOrCreateDevice()));
        auto copyBufferCmd = vsg::CopyAndReleaseBuffer::create(memoryBufferPools);

        QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

        auto psize = settings.value("POINTSIZE", 3).toInt();
        auto lodp = settings.value("LOD_POINTS", 0.1).toDouble();
        auto lodt = settings.value("LOD_TILES", 0.5).toDouble();

        sorter->setSourceModel(database->loadTiles(copyBufferCmd, lodt, lodp, psize));

        // compute the bounds of the scene graph to help position camera
        vsg::ComputeBounds computeBounds;
        computeBounds.traversalMask = route::SceneObjects | route::Tiles;
        database->getRoot()->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;


        auto horizonMountainHeight = settings.value("HMH", 0.0).toDouble();
        auto nearFarRatio = settings.value("NFR", 0.0001).toDouble();

        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + (vsg::normalize(centre)*10000.0), centre, vsg::dvec3(1.0, 0.0, 0.0));

        vsg::ref_ptr<vsg::ProjectionMatrix> perspective;

        if(!database)
            return false;

        vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
        if (ellipsoidModel)
        {
            perspective = vsg::EllipsoidPerspective::create(
                lookAt, ellipsoidModel, 60.0,
                static_cast<double>(width) /
                    static_cast<double>(height),
                nearFarRatio, horizonMountainHeight);
        }
        else
            return false;

        auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));



        // setup command graph to copy the image data each frame then rendering the scene graph
        auto grahics_commandGraph = vsg::CommandGraph::create(window);
        grahics_commandGraph->addChild(copyBufferCmd);
        grahics_commandGraph->addChild(vsg::createRenderGraphForView(window, camera, database->getRoot()));

        auto addBins = [&](vsg::View& view)
        {
            for (int bin = 0; bin < 11; ++bin)
                view.bins.push_back(vsg::Bin::create(bin, vsg::Bin::DESCENDING));
        };
        LambdaVisitor<decltype(addBins), vsg::View> lv(addBins);

        grahics_commandGraph->accept(lv);

        builder->setup(window, camera->viewportState);

        // add trackball to enable mouse driven camera view control.
        auto manipulator = Manipulator::create(camera, ellipsoidModel, database, this);

        viewer->addEventHandler(manipulator);

        viewer->assignRecordAndSubmitTaskAndPresentation({grahics_commandGraph});

        viewer->compile();

        connect(ui->actionSave, &QAction::triggered, database, &DatabaseManager::writeTiles);

        connect(sorter, &TilesSorter::doubleClicked, manipulator.get(), &Manipulator::moveToObject);

        connect(manipulator.get(), &Manipulator::sendIntersection, this, &MainWindow::intersection);

        connect(manipulator.get(), &Manipulator::sendPos, [this](const vsg::dvec3 &pos)
        {
            ui->cursorLat->setValue(pos.x);
            ui->cursorLon->setValue(pos.y);
            ui->cursorAlt->setValue(pos.z);
        });

        connect(ui->cursorLat, &QDoubleSpinBox::valueChanged, [this, manipulator](double value)
        {
            manipulator->setLatLongAlt(vsg::dvec3(value, ui->cursorLon->value(), ui->cursorAlt->value()));
        });
        connect(ui->cursorLon, &QDoubleSpinBox::valueChanged, [this, manipulator](double value)
        {
            manipulator->setLatLongAlt(vsg::dvec3(ui->cursorLat->value(), value, ui->cursorAlt->value()));
        });
        connect(ui->cursorAlt, &QDoubleSpinBox::valueChanged, [this, manipulator](double value)
        {
            manipulator->setLatLongAlt(vsg::dvec3(ui->cursorLat->value(), ui->cursorLon->value(), value));
        });

        connect(sorter, &TilesSorter::doubleClicked, manipulator.get(), &Manipulator::moveToObject);

        return true;
    };

    // provide the calls to invokve the vsg::Viewer to render a frame.
    viewerWindow->frameCallback = [this](vsgQt::ViewerWindow& vw) {

        if (!vw.viewer || !vw.viewer->advanceToNextFrame()) return false;

        // pass any events into EventHandlers assigned to the Viewer
        vw.viewer->handleEvents();

        vw.viewer->update();

        vw.viewer->recordAndSubmit();

        vw.viewer->present();

        return true;
    };
    return viewerWindow;
}
/*
void MainWindow::addObject()
{
    const auto selectedIndexes = ui->tilesView->selectionModel()->selectedIndexes();
    if(selectedIndexes.isEmpty() || !selectedIndexes.front().isValid())
        return;
    auto selected = static_cast<vsg::Node*>(sorter->mapToSource(selectedIndexes.front()).internalPointer());
    if(selected->is_compatible(typeid (vsg::Group)) || selected->is_compatible(typeid (vsg::Switch)))
    {
        AddDialog dialog(this);
        switch( dialog.exec() ) {
        case QDialog::Accepted:
        {
            //if(auto add = dialog.constructNode(); add)
                //undoStack->push(new AddNode(database->getTilesModel(), sorter->mapToSource(selectedIndexes.front()), add));
            break;
        }
        case QDialog::Rejected:
            break;
        default:
            break;
        }
    } else {
        QMessageBox msgBox;
        msgBox.setText("Пожалуйста, выберите группу сначала");
        msgBox.exec();
    }
}*/


void MainWindow::constructWidgets()
{
    embedded = QWidget::createWindowContainer(initilizeVSGwindow(), ui->centralsplitter);



    sorter = new TilesSorter(this);
    //sorter->setSourceModel();
    sorter->setFilterKeyColumn(1);
    sorter->setFilterWildcard("*");
    ui->tilesView->setModel(sorter);

    connect(ui->lineEdit, &QLineEdit::textChanged, sorter, &TilesSorter::setFilterWildcard);
    connect(ui->tilesView->selectionModel(), &QItemSelectionModel::selectionChanged, sorter, &TilesSorter::viewSelectSlot);
    connect(ui->tilesView, &QTreeView::doubleClicked, sorter, &TilesSorter::viewDoubleClicked);
    connect(sorter, &TilesSorter::viewSelectSignal, ui->tilesView->selectionModel(),
             qOverload<const QModelIndex &, QItemSelectionModel::SelectionFlags>(&QItemSelectionModel::select));
    connect(sorter, &TilesSorter::viewExpandSignal, ui->tilesView, &QTreeView::expand);

    ui->centralsplitter->addWidget(embedded);
    QList<int> sizes;
    sizes << 100 << 720;
    ui->centralsplitter->setSizes(sizes);
}

MainWindow::~MainWindow()
{
    delete ui;
}

