#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <vsgQt/ViewerWindow.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
#include "AddDialog.h"
#include "undo-redo.h"
#include "ObjectModel.h"
#include "LambdaVisitor.h"
#include "TilesVisitor.h"


MainWindow::MainWindow(QString routePath, QString skybox, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    undoStack = new QUndoStack(this);

    constructWidgets();

    database.reset(new DatabaseManager(routePath, undoStack, builder, fsmodel));
    sorter->setSourceModel(database->getTilesModel());
    scene->addChild(database->getDatabase());

    connect(sorter, &TilesSorter::selectionChanged, database.get(), &DatabaseManager::activeGroupChanged);
    connect(ui->fileView->selectionModel(), &QItemSelectionModel::selectionChanged, database.get(), &DatabaseManager::activeFileChanged);


    undoView = new QUndoView(undoStack, ui->tabWidget);
    ui->tabWidget->addTab(undoView, tr("Действия"));

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openRoute);
    connect(ui->addObjectButt, &QPushButton::pressed, this, &MainWindow::addObject);

}
QWindow* MainWindow::initilizeVSGwindow()
{
    options = vsg::Options::create();
    options->fileCache = vsg::getEnv("RRS2_CACHE");
    options->paths = vsg::getEnvPaths("RRS2_ROOT");

    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());
    vsg::RegisterWithObjectFactoryProxy<SceneObject>();

    builder = vsg::Builder::create();
    builder->options = options;

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = APPLICATION_NAME;
    if (settings.value("FULLSCREEN", false).toBool()) windowTraits->fullscreen = true;

    auto horizonMountainHeight = settings.value("HMH", 0.0).toDouble();
    auto nearFarRatio = settings.value("NFR", 0.0001).toDouble();

    scene = vsg::Group::create();

    SceneModel *scenemodel = new SceneModel(scene, undoStack, this);
    ui->sceneTreeView->setModel(scenemodel);
    ui->sceneTreeView->expandAll();

    viewerWindow = new vsgQt::ViewerWindow();
    viewerWindow->traits = windowTraits;
    viewerWindow->viewer = vsg::Viewer::create();

    // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
    viewerWindow->initializeCallback = [&, horizonMountainHeight, nearFarRatio](vsgQt::ViewerWindow& vw, uint32_t width, uint32_t height) {

        auto& window = vw.windowAdapter;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) viewer = vsg::Viewer::create();

        viewer->addWindow(window);

        // compute the bounds of the scene graph to help position camera
        vsg::ComputeBounds computeBounds;
        scene->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;


        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));

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
        {
            return false;
            /*
            perspective = vsg::Perspective::create(
                30.0,
                static_cast<double>(width) /
                    static_cast<double>(height),
                nearFarRatio * radius, radius * 4.5);
            */
        }

        auto objectModel = new ObjectModel(ellipsoidModel, undoStack);

        ui->tableView->setModel(objectModel);

        auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);

        auto addBins = [&](vsg::View& view)
        {
            for (int bin = 0; bin < 11; ++bin)
                view.bins.push_back(vsg::Bin::create(bin, vsg::Bin::DESCENDING));
        };
        LambdaVisitor<decltype(addBins), vsg::View> lv(addBins);
        commandGraph->accept(lv);

        // add trackball to enable mouse driven camera view control.
        auto manipulator = Manipulator::create(camera, ellipsoidModel, builder, scene, undoStack, database->getTilesModel());

        builder->setup(window, camera->viewportState);

        viewer->addEventHandler(manipulator);

        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

        viewer->compile();

        manipulator->setPager(viewer->recordAndSubmitTasks.front()->databasePager);
        database->setPager(viewer->recordAndSubmitTasks.front()->databasePager);

        connect(sorter, &TilesSorter::selectionChanged, objectModel, &ObjectModel::selectObject);

        connect(ui->actionAdd_model, &QAction::triggered, manipulator, &Manipulator::addAction);
        connect(ui->actionSave, &QAction::triggered, database.get(), &DatabaseManager::writeTiles);

        connect(manipulator.get(), &Manipulator::objectClicked, sorter, &TilesSorter::select);
        connect(manipulator.get(), &Manipulator::expand, sorter, &TilesSorter::expand);
        connect(manipulator.get(), &Manipulator::addRequest, database.get(), &DatabaseManager::addObject);
        connect(sorter, &TilesSorter::doubleClicked, manipulator, &Manipulator::selectObject);

        return true;
    };

    // provide the calls to invokve the vsg::Viewer to render a frame.
    viewerWindow->frameCallback = [](vsgQt::ViewerWindow& vw) {

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

void MainWindow::addObject()
{
    const auto selectedIndexes = ui->tilesView->selectionModel()->selectedIndexes();
    if(selectedIndexes.isEmpty())
        return;
    auto selected = static_cast<vsg::Node*>(selectedIndexes.front().internalPointer());
    if(auto group = selected->cast<vsg::Group>(); group)
    {
        AddDialog dialog(this);
        switch( dialog.exec() ) {
        case QDialog::Accepted:
        {
            auto add = dialog.constructCommand(group);
            database->getTilesModel()->addNode(selectedIndexes.front(), add);
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
}
DatabaseManager *MainWindow::openDialog()
{
    if (const auto file = QFileDialog::getOpenFileName(this, tr("Открыть базу данных"), qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "routes"); !file.isEmpty())
    {
        try {
            auto db = new DatabaseManager(file, undoStack, builder, fsmodel);
            return db;

        }  catch (DatabaseException &ex) {
            auto errorMessageDialog = new QErrorMessage(this);
            errorMessageDialog->showMessage(ex.getErrPath());
        }
    }
    return nullptr;
}

void MainWindow::openRoute()
{
    if(auto manager = openDialog(); manager)
    {
        scene->children.clear();
        database.reset(manager);
        sorter->setSourceModel(manager->getTilesModel());
        scene->addChild(manager->getDatabase());

        connect(sorter, &TilesSorter::selectionChanged, database.get(), &DatabaseManager::activeGroupChanged);
        connect(ui->fileView->selectionModel(), &QItemSelectionModel::selectionChanged, database.get(), &DatabaseManager::activeFileChanged);

        viewerWindow->viewer = vsg::Viewer::create();
        viewerWindow->initializeCallback(*viewerWindow, embedded->width(), embedded->height());
    }
}


void MainWindow::pushCommand(QUndoCommand *command)
{
    undoStack->push(command);
}

void MainWindow::constructWidgets()
{
    embedded = QWidget::createWindowContainer(initilizeVSGwindow(), ui->centralsplitter);

    auto model = new QFileSystemModel(this);
    ui->fileView->setModel(model);
    ui->fileView->setRootIndex(model->setRootPath(qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "objects"));

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

