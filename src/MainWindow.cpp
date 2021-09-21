#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <vsgXchange/all.h>
#include <vsgQt/ViewerWindow.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QSettings>
#include "databasemanager.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    constructWidgets();

    connect(ui->actionOpen, &QAction::triggered, this, [=]() {
        scene->children.clear();
        if (const auto filename = QFileDialog::getOpenFileName(this, tr("Open file"), nullptr, "VSG bin files (*.vsgb);;All files (*.*)"); !filename.isEmpty())
        {
            database.reset(new DatabaseManager(filename, options.get()));
            tilestree->setModel(database->getTilesModel());
            connect(database.data(), &DatabaseManager::emitTilesRoot, tilestree, &QTreeView::setRootIndex);
            if( auto root = database->loadDatabase(); root)
                addToRoot(root);
        }
    });
    connect(ui->actionUpdate, &QAction::triggered, this, [=]() {
        scenetree->setRootIndex(scenetree->model()->index(0,0));
    });

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

    auto scenemodel = new SceneModel(scene, this);
    scenetree->setModel(scenemodel);
    scenetree->setRootIndex(scenemodel->index(0,0));

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
        vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(scene->getObject<vsg::EllipsoidModel>("EllipsoidModel"));
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
        trackball = vsg::Trackball::create(camera, ellipsoidModel);
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

void MainWindow::addToRoot(vsg::Node *node)
{
    scene->addChild(vsg::ref_ptr<vsg::Node>(node));
    viewerWindow->viewer = vsg::Viewer::create();
    viewerWindow->initializeCallback(*viewerWindow);
    scenetree->setRootIndex(scenetree->model()->index(0,0));

}


void MainWindow::constructWidgets()
{
    QWidget *centralWidget = new QWidget( this );
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    QGridLayout *baseWidgetLayout = new QGridLayout( centralWidget );
    baseWidgetLayout->setSpacing(0);
    baseWidgetLayout->setContentsMargins(0, 0, 0, 0);
    baseWidgetLayout->setObjectName("gridLayout");

    //treeview->setItemDelegateForColumn(0, )
    centralsplitter = new QSplitter( this );
    auto analyzers = new QTabWidget(centralsplitter);
    scenetree = new QTreeView(analyzers);
    scenetree->setAllColumnsShowFocus(true);
    tilestree = new QTreeView(analyzers);
    tilestree->setAllColumnsShowFocus(true);
    analyzers->addTab(scenetree, "Scene");
    analyzers->addTab(tilestree, "Objects");

    auto widget = QWidget::createWindowContainer(initilizeVSGwindow(), this);

    centralsplitter->setObjectName("Centralsplitter");
    centralsplitter->setOrientation( Qt::Horizontal );
    centralsplitter->addWidget( widget );
    centralsplitter->addWidget( analyzers );

    baseWidgetLayout->addWidget( centralsplitter, 0, 0 );

    QList<int> sizes;
    sizes << 350 << 500;
    centralsplitter->setSizes( sizes );
}

MainWindow::~MainWindow()
{
    delete ui;
}

