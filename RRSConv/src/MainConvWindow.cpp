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

    if (const auto file = QFileDialog::getOpenFileName(this, tr("Загрузить модели"), qgetenv("RRS2_ROOT")); !file.isEmpty())
    {
        scene = vsg::read_cast<vsg::Node>(file.toStdString(), options);
    }

}
QWindow* MainWindow::initilizeVSGwindow()
{
    // set up vsg::Options to pass in filepaths and ReaderWriter's and other IO
    // realted options to use when reading and writing files.
    options = vsg::Options::create();
    options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

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
                static_cast<double>(width) /
                    static_cast<double>(height),
                nearFarRatio, 0.0);
        }
        else
        {
            perspective = vsg::Perspective::create(
                30.0,
                static_cast<double>(width) /
                    static_cast<double>(height),
                nearFarRatio * radius, radius * 4.5);
        }

        auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        // add trackball to enable mouse driven camera view control.
        viewer->addEventHandler(vsg::Trackball::create(camera, ellipsoidModel));

        auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

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
    QList<int> sizes;
    sizes << 100 << 720;
    ui->centralsplitter->setSizes(sizes);
}

MainWindow::~MainWindow()
{
    delete ui;
}
