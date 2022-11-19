#include "MainConvWindow.h"
#include "ui_MainConvWindow.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
//#include "LambdaVisitor.h"

MainWindow::MainWindow( QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    constructWidgets();

    //scene = vsg::Group::create();

    /*if (const auto file = QFileDialog::getOpenFileName(this, tr("Загрузить модели"), qgetenv("RRS2_ROOT")); !file.isEmpty())
    {
        auto model = vsg::read_cast<vsg::Node>(file.toStdString(), options);
        if(model)
            scene->addChild(model);
    }*/
    scene->addChild(vsg::read_cast<vsg::Node>("/home/asafr/RRS/objects/trackside/signals/Выходной.dae", options));

}

vsg::ref_ptr<vsg::Camera> createCameraForScene(vsg::Node* scenegraph, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    // compute the bounds of the scene graph to help position camera
    vsg::ComputeBounds computeBounds;
    scenegraph->accept(computeBounds);
    vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;
    double nearFarRatio = 0.001;

    // set up the camera
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0),
                                      centre, vsg::dvec3(0.0, 0.0, 1.0));

    auto perspective = vsg::Perspective::create(30.0, static_cast<double>(width) / static_cast<double>(height),
                                                nearFarRatio * radius, radius * 4.5);

    auto viewportstate = vsg::ViewportState::create(x, y, width, height);

    return vsg::Camera::create(perspective, lookAt, viewportstate);
}

QWindow* MainWindow::initilizeVSGwindow()
{
    // set up vsg::Options to pass in filepaths and ReaderWriter's and other IO
    // realted options to use when reading and writing files.
    options = vsg::Options::create();

    scene = vsg::Group::create();
    model = AnimatedModel::create();

    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "vsgQt viewer";

    viewerWindow = new vsgQt::ViewerWindow();

    // if required set the QWindow's SurfaceType to QSurface::VulkanSurface.
    viewerWindow->setSurfaceType(QSurface::VulkanSurface);

    viewerWindow->traits = windowTraits;

    viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw, uint32_t width, uint32_t height)
    {

        auto& window = vw.windowAdapter;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) viewer = vsg::Viewer::create();

        vsg::RegisterWithObjectFactoryProxy<route::SceneObject>();

        viewer->addWindow(window);

        // create the vsg::RenderGraph and associated vsg::View
        auto main_camera = createCameraForScene(scene, 0, 0, width / 2, height);
        auto main_view = vsg::View::create(main_camera, scene);
        main_view->addChild(vsg::createHeadlight());

        // create an RenderinGraph to add an secondary vsg::View on the top right part of the window.
        auto secondary_camera = createCameraForScene(scene, width / 2, 0, width / 2, height);
        auto secondary_view = vsg::View::create(secondary_camera, model);
        secondary_view->addChild(vsg::createHeadlight());

        handler->camera = main_camera;

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        // add trackball to enable mouse driven camera view control.
        viewer->addEventHandler(vsg::Trackball::create(secondary_camera));
        viewer->addEventHandler(vsg::Trackball::create(main_camera));

        viewer->addEventHandler(handler);

        auto main_RenderGraph = vsg::RenderGraph::create(window, main_view);
        auto secondary_RenderGraph = vsg::RenderGraph::create(window, secondary_view);
        secondary_RenderGraph->clearValues[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};

        auto commandGraph = vsg::CommandGraph::create(window);
        commandGraph->addChild(main_RenderGraph);
        commandGraph->addChild(secondary_RenderGraph);

        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

        handler->builder->assignCompileTraversal(vsg::CompileTraversal::create(*viewer));

        viewer->compile();

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

}
*/
void MainWindow::constructWidgets()
{
    embedded = QWidget::createWindowContainer(initilizeVSGwindow(), ui->centralsplitter);
    ui->centralsplitter->addWidget(embedded);

    handler = new IntersectionHandler(scene, model, ui->centralsplitter);
    ui->centralsplitter->addWidget(handler);

    handler->builder = vsg::Builder::create();
    handler->builder->options = options;

    QList<int> sizes;
    sizes << 100 << 720 << 100;
    ui->centralsplitter->setSizes(sizes);
}

MainWindow::~MainWindow()
{
    delete ui;
}
