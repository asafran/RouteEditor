#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <vsgQt/ViewerWindow.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
#include "manipulator.h"
#include "AddDialog.h"
#include "undo-redo.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    undoStack = new QUndoStack(this);

    constructWidgets();

    undoView = new QUndoView(undoStack, ui->tabWidget);
    ui->tabWidget->addTab(undoView, tr("Действия"));

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openRoute);

    connect(ui->addObjectButt, &QPushButton::pressed, this, &MainWindow::addObject);
}
QWindow* MainWindow::initilizeVSGwindow()
{
    options = vsg::Options::create();
    //options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());

    builder = vsg::Builder::create();
    builder->options = options;

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = APPLICATION_NAME;
    if (settings.value("FULLSCREEN", false).toBool()) windowTraits->fullscreen = true;

    auto horizonMountainHeight = settings.value("HMH", 0.0).toDouble();

    scene = vsg::Group::create();

    vsg::GeometryInfo geomInfo;
    geomInfo.dx.set(1.0f, 0.0f, 0.0f);
    geomInfo.dy.set(0.0f, 1.0f, 0.0f);
    geomInfo.dz.set(0.0f, 0.0f, 1.0f);

    vsg::StateInfo stateInfo;

    scene->addChild(builder->createBox(geomInfo, stateInfo));

    SceneModel *scenemodel = new SceneModel(scene, undoStack, this);
    ui->sceneTreeView->setModel(scenemodel);
    ui->sceneTreeView->expandAll();

    viewerWindow = new vsgQt::ViewerWindow();
    viewerWindow->traits = windowTraits;
    viewerWindow->viewer = vsg::Viewer::create();

    // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
    viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw) {

        auto& window = vw.proxyWindow;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) return false;

        viewer->addWindow(window);

        // compute the bounds of the scene graph to help position camera
        vsg::ComputeBounds computeBounds;
        scene->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;
        double nearFarRatio = 0.001;

        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));

        vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
        vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(scene->children.front()->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
        if (ellipsoidModel)
        {
            perspective = vsg::EllipsoidPerspective::create(
                lookAt, ellipsoidModel, 30.0,
                static_cast<double>(window->extent2D().width) /
                    static_cast<double>(window->extent2D().height),
                nearFarRatio, horizonMountainHeight);
        }
        else
        {
            perspective = vsg::Perspective::create(
                30.0,
                static_cast<double>(window->extent2D().width) /
                    static_cast<double>(window->extent2D().height),
                nearFarRatio * radius, radius * 4.5);
        }

        auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        // add trackball to enable mouse driven camera view control.
        auto trackball = Manipulator::create(camera, ellipsoidModel, builder, scene, radius * 0.1, options);

        viewer->addEventHandler(trackball);

        auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

        viewer->compile();

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

void MainWindow::addToRoot(vsg::ref_ptr<vsg::Node> node)
{
    scene->addChild(node);
    viewerWindow->viewer = vsg::Viewer::create();
    viewerWindow->initializeCallback(*viewerWindow);
    ui->sceneTreeView->setRootIndex(ui->sceneTreeView->model()->index(0,0));

}
void MainWindow::addObject()
{
    const auto selectedIndexes = ui->cachedTilesView->selectionModel()->selectedIndexes();
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
            database->getCahedTilesModel()->addNode(selectedIndexes.front(), add);
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

void MainWindow::openRoute()
{
    scene->children.clear();

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    if (const auto file = QFileDialog::getOpenFileName(this, tr("Открыть базу данных"), settings.value("ROUTES", qApp->applicationDirPath()).toString()); !file.isEmpty())
    {
        try {
            database.reset(new DatabaseManager(file, undoStack));
            ui->loadedTilesView->setModel(database->getLoadedTilesModel());
            ui->cachedTilesView->setModel(database->getCahedTilesModel());
            QObject::connect(database.get(), &DatabaseManager::updateViews, ui->loadedTilesView, &QTreeView::expandAll);
            QObject::connect(database.get(), &DatabaseManager::updateViews, ui->cachedTilesView, &QTreeView::expandAll);
            QObject::connect(ui->updateButt, &QPushButton::pressed, database.get(), &DatabaseManager::updateTileCache);
            QObject::connect(ui->updateFilesButt, &QPushButton::pressed, database.get(), &DatabaseManager::loadTiles);
            addToRoot(database->getDatabase());

        }  catch (DatabaseException &ex) {
            auto errorMessageDialog = new QErrorMessage(this);
            errorMessageDialog->showMessage(ex.getErrPath());
        }
    }
}

void MainWindow::pushCommand(QUndoCommand *command)
{
    undoStack->push(command);
}

void MainWindow::constructWidgets()
{

    auto widged = QWidget::createWindowContainer(initilizeVSGwindow(), ui->centralsplitter);
    ui->centralsplitter->addWidget(widged);
    QList<int> sizes;
        sizes << 100 << 720;
        ui->centralsplitter->setSizes(sizes);
}

MainWindow::~MainWindow()
{
    delete ui;
}

