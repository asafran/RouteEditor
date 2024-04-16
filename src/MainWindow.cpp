#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <vsgQt/Window.h>

#include <QFileDialog>
#include <QColorDialog>
#include <QErrorMessage>
#include <QMessageBox>
#include "undo-redo.h"
#include "InterlockDialog.h"
#include "ContentManager.h"
#include <QVulkanInstance>

#include "Painter.h"

#include "InverseMatrices.h"

MainWindow::MainWindow(vsg::ref_ptr<DatabaseManager> dbm, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _database(dbm)
{

    ui->setupUi(this);

    constructWidgets();

    _database->setUndoStack(new QUndoStack(this));

    initializeTools();

    _undoView = new QUndoView(_database->undoStack, ui->tabWidget);
    ui->tabWidget->addTab(_undoView, tr("Действия"));

    connect(ui->actionUndo, &QAction::triggered, _database->undoStack, &QUndoStack::undo);
    connect(ui->actionRedo, &QAction::triggered, _database->undoStack, &QUndoStack::redo);

    connect(ui->actionSave, &QAction::triggered, this, [this](){ _database->writeTiles(); });

    connect(ui->removeButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = _sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            if(selected.size() > 1)
            {
                std::sort(selected.begin(), selected.end(), [](auto lhs, auto rhs){ return lhs.row() > rhs.row(); });
                _database->undoStack->beginMacro("Удалены объекты");
            }

            for (const auto &index : selected)
                    _database->undoStack->push(new RemoveNode(_database->tilesModel, index));

            if(selected.size() > 1)
                _database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты, которые нужно удалить"), 3000);
    });
    connect(ui->addGroupButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = _sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            _database->undoStack->beginMacro(tr("Создан слой"));
            auto group = route::SceneGroup::create();
            auto parent = selected.front().parent();
            for (const auto &index : selected)
            {
                Q_ASSERT(index.isValid());
                //group->childrenObjects().emplace_back(static_cast<route::MVCObject*>(index.internalPointer()));
                _database->undoStack->push(new RemoveNode(_database->tilesModel, index));
            }
            _database->undoStack->push(new AddSceneObject(_database->tilesModel, parent, group));
            _database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты для создания слоя"), 3000);

    });
    connect(ui->addSObjectButt, &QPushButton::pressed, this, [this]()
    {
        auto selected = _sorter->mapSelectionToSource(ui->tilesView->selectionModel()->selection()).indexes();
        if(!selected.empty())
        {
            auto front = selected.front();
            auto parentIndex = front.parent();
            auto parent = static_cast<vsg::Node*>(parentIndex.internalPointer());
            Q_ASSERT(parent);

            auto node = static_cast<vsg::Node*>(front.internalPointer());
            auto object = node->cast<route::SceneObject>();
            if(!object)
            {
                ui->statusbar->showMessage(tr("Выберите первым объект"), 2000);
                return;
            }

            //auto position = object->getWorldPosition();
            //auto group = route::SceneObject::create(database->getStdWireBox(), object->getTransform());

            _database->undoStack->beginMacro(tr("Создана группа объектов"));

            std::sort(selected.begin(), selected.end(), [](auto lhs, auto rhs){ return lhs.row() > rhs.row(); });

            for (const auto &index : selected)
            {
                Q_ASSERT(index.isValid());
                auto node = static_cast<vsg::Node*>(index.internalPointer());
                auto object = node->cast<route::SceneObject>();
                if(!object)
                {
                    ui->statusbar->showMessage(tr("Поддерживается перемещение только объектов"), 2000);
                    continue;
                }
                //object->reset();
                //object->setPosition(position - object->getWorldPosition());

                //group->children.emplace_back(object);
                _database->undoStack->push(new RemoveNode(_database->tilesModel, index));
            }

            //database->undoStack->push(new AddSceneObject(database->tilesModel, parentIndex, group));
            _database->undoStack->endMacro();
        }
        else
            ui->statusbar->showMessage(tr("Выберите объекты для создания слоя"), 3000);
    });

    connect(ui->actionSig, &QAction::triggered, this, [this]()
    {
        InterlockDialog dialog(_database, this);
        dialog.exec();
    });
}


void MainWindow::initializeTools()
{
    QSettings settings(app::ORGANIZATION_NAME, app::APP_NAME);

    _toolbox = new QToolBox(ui->splitter);

    auto contentRoot = qgetenv("RRS2_ROOT");

    _objectsPrpEditor = new ObjectPropertiesEditor(_database, ui->splitter);
    ui->splitter->addWidget(_objectsPrpEditor);
    ui->splitter->addWidget(_toolbox);

    _railsPointEditor = new RailsPointEditor(_database, _toolbox);
    _toolbox->addItem(_railsPointEditor, tr("Изменить параметры путеовй точки"));
    _contentManager = new ContentManager(_database, contentRoot + "/objects/objects", _toolbox);
    _toolbox->addItem(_contentManager, tr("Добавить объект"));
    _railsManager = new AddRails(_database, contentRoot + "/objects/rails", _toolbox);
    _toolbox->addItem(_railsManager, tr("Добавить рельсы"));
    _painter = new Painter(_database, contentRoot + "/textures", _toolbox);
    _toolbox->addItem(_painter, tr("Текстурирование"));

    connect(_sorter, &TilesSorter::selectionChanged, _objectsPrpEditor, &ObjectPropertiesEditor::selectIndex);
    connect(_objectsPrpEditor, &ObjectPropertiesEditor::objectClicked, _sorter, &TilesSorter::select);
    connect(_objectsPrpEditor, &ObjectPropertiesEditor::deselect, ui->tilesView->selectionModel(), &QItemSelectionModel::clear);
    connect(_objectsPrpEditor, &ObjectPropertiesEditor::deselectItem, _sorter, &TilesSorter::deselect);
    connect(_contentManager, &ContentManager::sendObject, _objectsPrpEditor, &ObjectPropertiesEditor::selectObject);
    connect(_railsManager, &AddRails::sendMovingPoint, _objectsPrpEditor, &ObjectPropertiesEditor::selectObject);
    connect(_toolbox, &QToolBox::currentChanged, _objectsPrpEditor, &ObjectPropertiesEditor::clearSelection);
    connect(_toolbox, &QToolBox::currentChanged, _railsPointEditor, &RailsPointEditor::clearSelection);
    connect(_sorter, &TilesSorter::frontSelectionChanged, _contentManager, &ContentManager::activeGroupChanged);
}

vsgQt::Window* MainWindow::createWindow()
{
    auto traits = vsg::WindowTraits::create();

    auto window = new vsgQt::Window(_viewer, traits);

    window->setTitle(app::APP_NAME);

    window->initializeWindow();

    QSettings settings(app::ORGANIZATION_NAME, app::APP_NAME);

    // if this is the first window to be created, use its device for future window creation.
    if (!traits->device) traits->device = window->windowAdapter->getOrCreateDevice();

    auto horizonMountainHeight = settings.value("HMH", 0.0).toDouble();
    auto nearFarRatio = settings.value("NFR", 0.0001).toDouble();

    uint32_t width = window->traits->width;
    uint32_t height = window->traits->height;

    auto ellipsoidModel = _database->route->atmosphere->ellipsoidModel;
    auto atmosphere = _database->route->atmosphere;

    auto lookAt = _database->lastLookAt;
    auto perspective = vsg::EllipsoidPerspective::create(
        lookAt, ellipsoidModel, 30.0,
        static_cast<double>(width) /
            static_cast<double>(height),
        nearFarRatio, horizonMountainHeight);
    auto camera = vsg::Camera::create(perspective, _database->lastLookAt, vsg::ViewportState::create(VkExtent2D{width, height}));

    // create the sky camera
    auto inversePerojection = atmosphere::InverseProjection::create(camera->projectionMatrix);
    auto inverseView = atmosphere::InverseView::create(camera->viewMatrix);
    auto skyCamera = vsg::Camera::create(inversePerojection, inverseView, vsg::ViewportState::create(window->windowAdapter->extent2D()));

    _database->opThreads = vsg::OperationThreads::create(QThread::idealThreadCount(), _viewer->status);

    // add close handler to respond the close window button and pressing escape
    _viewer->addEventHandler(vsg::CloseHandler::create(_viewer));

    auto l = vsg::AmbientLight::create();
    l->intensity = 0.5f;

    auto mainView = vsg::View::create(camera);
    auto skyView = vsg::View::create(skyCamera);

    auto mainViewDependent = atmosphere::AtmosphereLighting::create(mainView, atmosphere);
    mainViewDependent->exposure = 5.0;
    auto skyViewDependent = atmosphere::SkyLighting::create(skyView, atmosphere);
    skyViewDependent->exposure = 3.0;

    mainView->addChild(_database->root);
    mainView->addChild(l);
    auto sky = atmosphere->createSky();
    skyView->addChild(sky);

    atmosphere->setSunAngle(150.0);

    //_database->builder->options->shaderSets["phong"] = atmosphere->phongShaderSet;

    mainView->viewDependentState = mainViewDependent;
    skyView->viewDependentState = skyViewDependent;

    // set up the render graph
    auto renderGraph = vsg::RenderGraph::create(*window, mainView);
    renderGraph->contents = VK_SUBPASS_CONTENTS_INLINE;

    renderGraph->addChild(skyView);
    renderGraph->setClearValues({{0.0f, 0.0f, 0.0f, 1.0f}});

    auto grahics_commandGraph = vsg::CommandGraph::create(*window, renderGraph);
    _viewer->assignRecordAndSubmitTaskAndPresentation({grahics_commandGraph});

    // add trackball to enable mouse driven camera view control.
    auto manipulator = Manipulator::create(camera, ellipsoidModel, _database, this);

    vsg::EventHandlers handlers{manipulator, vsg::CloseHandler::create(_viewer)};
    handlers.emplace_back(_contentManager);
    handlers.emplace_back(_objectsPrpEditor);
    handlers.emplace_back(_railsPointEditor);
    handlers.emplace_back(_railsManager);
    handlers.emplace_back(_painter);
    _viewer->addEventHandlers(std::move(handlers));

    vsg::Path resource = std::string(qgetenv("RRS2_ROOT")) + QDir::separator().toLatin1() + "resource.vsgt";
    auto resourceHints = vsg::read_cast<vsg::ResourceHints>(resource);

    if (!resourceHints)
    {
        // To help reduce the number of vsg::DescriptorPool that need to be allocated we'll provide a minimum requirement via ResourceHints.
        resourceHints = vsg::ResourceHints::create();
        resourceHints->numDescriptorSets = 256;
        resourceHints->descriptorPoolSizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256});
    }

    auto trackball = vsg::Trackball::create(camera, ellipsoidModel);
    trackball->addWindow(*window);

    _viewer->addEventHandler(trackball);

    return window;
}


void MainWindow::constructWidgets()
{
    _embedded = QWidget::createWindowContainer(createWindow(), ui->centralsplitter);

    _sorter = new TilesSorter(this);
    _sorter->setSourceModel(_database->tilesModel);
    _sorter->setFilterKeyColumn(1);
    _sorter->setFilterWildcard("*");
    ui->tilesView->setModel(_sorter);

    connect(ui->lineEdit, &QLineEdit::textChanged, _sorter, &TilesSorter::setFilterWildcard);
    connect(ui->tilesView->selectionModel(), &QItemSelectionModel::selectionChanged, _sorter, &TilesSorter::viewSelectSlot);
    connect(ui->tilesView, &QTreeView::doubleClicked, _sorter, &TilesSorter::viewDoubleClicked);
    connect(_sorter, &TilesSorter::viewSelectSignal, ui->tilesView->selectionModel(),
             qOverload<const QModelIndex &, QItemSelectionModel::SelectionFlags>(&QItemSelectionModel::select));
    connect(_sorter, &TilesSorter::viewExpandSignal, ui->tilesView, &QTreeView::expand);

    ui->centralsplitter->addWidget(_embedded);
    QList<int> sizes;
    sizes << 100 << 720;
    ui->centralsplitter->setSizes(sizes);
}

MainWindow::~MainWindow()
{
    delete ui;
}

