#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <vsgQt/ViewerWindow.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
#include "undo-redo.h"
#include "InterlockDialog.h"
#include "LambdaVisitor.h"
#include "ParentVisitor.h"
#include "ContentManager.h"

#include "Painter.h"



MainWindow::MainWindow(vsg::ref_ptr<DatabaseManager> dbm, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , database(dbm)
{

    ui->setupUi(this);

    constructWidgets();

    database->setUndoStack(new QUndoStack(this));

    initializeTools();

    undoView = new QUndoView(database->undoStack, ui->tabWidget);
    ui->tabWidget->addTab(undoView, tr("Действия"));

    connect(ui->actionUndo, &QAction::triggered, database->undoStack, &QUndoStack::undo);
    connect(ui->actionRedo, &QAction::triggered, database->undoStack, &QUndoStack::redo);

    connect(ui->removeButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            if(selected.size() != 1)
            {
                std::sort(selected.begin(), selected.end(), [](auto lhs, auto rhs){ return lhs.row() > rhs.row(); });
                database->undoStack->beginMacro("Удалены объекты");
            }

            for (const auto &index : selected)
                    database->undoStack->push(new RemoveNode(database->tilesModel, index));

            if(selected.size() != 1)
                database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты, которые нужно удалить"), 3000);
    });
    connect(ui->addGroupButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            database->undoStack->beginMacro(tr("Создан слой"));
            auto group = vsg::Group::create();
            auto parent = selected.front().parent();
            for (const auto &index : selected)
            {
                Q_ASSERT(index.isValid());
                group->children.emplace_back(static_cast<vsg::Node*>(index.internalPointer()));
                database->undoStack->push(new RemoveNode(database->tilesModel, index));
            }
            database->undoStack->push(new AddSceneObject(database->tilesModel, parent, group));
            database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты для создания слоя"), 3000);

    });
    connect(ui->addSObjectButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            auto front = selected.front();
            auto parentIndex = front.parent();
            auto parent = static_cast<vsg::Node*>(parentIndex.internalPointer());
            Q_ASSERT(parent);

            ParentTracer pt;
            parent->accept(pt);
            auto ltw = vsg::computeTransform(pt.nodePath);
            auto wtl = vsg::inverse(ltw);

            auto node = static_cast<vsg::Node*>(front.internalPointer());
            auto object = node->cast<route::SceneObject>();
            if(!object)
            {
                ui->statusbar->showMessage(tr("Выберите первым объект"), 2000);
                return;
            }

            auto world = object->getWorldPosition();
            auto norm = vsg::normalize(world);
            //vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);
            auto group = route::SceneObject::create(database->getStdWireBox(), world * wtl, vsg::dquat(), wtl);

            database->undoStack->beginMacro(tr("Создана группа объектов"));

            std::sort(selected.begin(), selected.end(), [](auto lhs, auto rhs){ return lhs.row() > rhs.row(); });

            for (const auto &index : selected)
            {
                Q_ASSERT(index.isValid());
                auto object = static_cast<vsg::Node*>(index.internalPointer());
                group->children.emplace_back(object);
                database->undoStack->push(new RemoveNode(database->tilesModel, index));
            }

            database->undoStack->push(new AddSceneObject(database->tilesModel, parentIndex, group));

            CalculateTransform ct;
            ct.undoStack = database->undoStack;
            ct.stack.push(ltw);
            group->accept(ct);

            database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты для создания слоя"), 3000);
    });

    connect(ui->actionSig, &QAction::triggered, this, [this]()
    {
        InterlockDialog dialog(database, this);
        dialog.exec();
    });
}


void MainWindow::initializeTools()
{
    QSettings settings(app::ORGANIZATION_NAME, app::APPLICATION_NAME);

    toolbox = new QToolBox(ui->splitter);
    ui->splitter->addWidget(toolbox);

    auto contentRoot = qgetenv("RRS2_ROOT");

    ope = new ObjectPropertiesEditor(database, toolbox);
    toolbox->addItem(ope, tr("Выбрать и переместить объекты"));
    auto rpe = new RailsPointEditor(database, toolbox);
    toolbox->addItem(rpe, tr("Изменить параметры путеовй точки"));
    auto cm = new ContentManager(database, contentRoot + "/objects/objects", toolbox);
    toolbox->addItem(cm, tr("Добавить объект"));
    //auto sm = new SignalManager(database, contentRoot + "/objects/trackside/signals/");
    //toolbox->addItem(sm, tr("Добавить сигнал"));
    rm = new AddRails(database, contentRoot + "/objects/rails", toolbox);
    toolbox->addItem(rm, tr("Добавить рельсы"));
    auto pt = new Painter(database, contentRoot + "/textures", toolbox);
    toolbox->addItem(pt, tr("Текстурирование"));

    connect(sorter, &TilesSorter::selectionChanged, ope, &ObjectPropertiesEditor::selectIndex);
    connect(ope, &ObjectPropertiesEditor::objectClicked, sorter, &TilesSorter::select);
    connect(ope, &ObjectPropertiesEditor::deselect, ui->tilesView->selectionModel(), &QItemSelectionModel::clear);
    connect(ope, &ObjectPropertiesEditor::deselectItem, sorter, &TilesSorter::deselect);
    connect(cm, &ContentManager::sendObject, ope, &ObjectPropertiesEditor::selectObject);
    connect(rm, &AddRails::sendMovingPoint, ope, &ObjectPropertiesEditor::selectObject);
    connect(toolbox, &QToolBox::currentChanged, ope, &ObjectPropertiesEditor::clearSelection);
    connect(toolbox, &QToolBox::currentChanged, rpe, &RailsPointEditor::clearSelection);
    connect(sorter, &TilesSorter::frontSelectionChanged, cm, &ContentManager::activeGroupChanged);
}

void MainWindow::intersection(const FoundNodes &isection)
{
    qobject_cast<Tool*>(toolbox->currentWidget())->intersection(isection);
}

QWindow* MainWindow::initilizeVSGwindow()
{

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = app::APPLICATION_NAME;

    /*
    SceneModel *scenemodel = new SceneModel(database->getRoot(), this);
    ui->sceneTreeView->setModel(scenemodel);
    ui->sceneTreeView->expandAll();
*/
    viewerWindow = new vsgQt::ViewerWindow();
    viewerWindow->setSurfaceType(QSurface::VulkanSurface);
    viewerWindow->traits = windowTraits;
    viewerWindow->viewer = vsg::Viewer::create();

    // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
    viewerWindow->initializeCallback = [&, this](vsgQt::ViewerWindow& vw, uint32_t width, uint32_t height) {

        auto& window = vw.windowAdapter;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) viewer = vsg::Viewer::create();

        viewer->addWindow(window);

        auto memoryBufferPools = vsg::MemoryBufferPools::create("Staging_MemoryBufferPool", vsg::ref_ptr<vsg::Device>(window->getOrCreateDevice()));
        database->copyImageCmd = vsg::CopyAndReleaseImage::create(memoryBufferPools);

        QSettings settings(app::ORGANIZATION_NAME, app::APPLICATION_NAME);

        sorter->setSourceModel(database->tilesModel);

        // compute the bounds of the scene graph to help position camera
        vsg::ComputeBounds computeBounds;
        computeBounds.traversalMask = route::SceneObjects | route::Tiles;
        database->root->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        //double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;

        auto horizonMountainHeight = settings.value("HMH", 0.0).toDouble();
        auto nearFarRatio = settings.value("NFR", 0.0001).toDouble();

        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + (vsg::normalize(centre)*1000.0), centre, vsg::dvec3(1.0, 0.0, 0.0));
        //auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));

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

        auto view = vsg::View::create(camera);
        view->addChild(vsg::createHeadlight());
        view->addChild(database->root);

        // setup command graph to copy the image data each frame then rendering the scene graph
        auto grahics_commandGraph = vsg::CommandGraph::create(window);
        grahics_commandGraph->addChild(database->copyImageCmd);
        grahics_commandGraph->addChild(vsg::RenderGraph::create(window, view));

        // add trackball to enable mouse driven camera view control.
        auto manipulator = Manipulator::create(camera, ellipsoidModel, database, this);

        auto addBins = [&](vsg::View& view)
        {
            for (int bin = 0; bin < 11; ++bin)
                view.bins.push_back(vsg::Bin::create(bin, vsg::Bin::DESCENDING));
        };
        LambdaVisitor<decltype(addBins), vsg::View> lv(addBins);

        grahics_commandGraph->accept(lv);

        viewer->addEventHandler(manipulator);

        viewer->assignRecordAndSubmitTaskAndPresentation({grahics_commandGraph});

        vsg::Path resource = std::string(qgetenv("RRS2_ROOT")) + QDir::separator().toLatin1() + "resource.vsgt";
        auto resourceHints = vsg::read_cast<vsg::ResourceHints>(resource);

        if (!resourceHints)
        {
            // To help reduce the number of vsg::DescriptorPool that need to be allocated we'll provide a minimum requirement via ResourceHints.
            resourceHints = vsg::ResourceHints::create();
            resourceHints->numDescriptorSets = 256;
            resourceHints->descriptorPoolSizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256});
        }

        // configure the viewers rendering backend, initialize and compile Vulkan objects, passing in ResourceHints to guide the resources allocated.
        viewer->compile(resourceHints);

        connect(ui->actionSave, &QAction::triggered, this, [this](){ database->writeTiles(); });

        connect(sorter, &TilesSorter::doubleClicked, manipulator.get(), &Manipulator::moveToObject);

        connect(rm, &AddRails::startMoving, manipulator.get(), &Manipulator::startMoving);

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

        connect(ope, &ObjectPropertiesEditor::sendFirst, manipulator, &Manipulator::setFirst);
        connect(manipulator, &Manipulator::sendMovingDelta, ope, &ObjectPropertiesEditor::move);

        database->setViewer(viewer);

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

