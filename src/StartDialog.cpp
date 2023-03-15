#include "StartDialog.h"
#include "route.h"
#include "ui_StartDialog.h"
#include <QSettings>
#include <vsg/io/read.h>
#include <vsgXchange/all.h>
#include "Constants.h"
#include "Register.h"
#include <execution>

StartDialog::StartDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartDialog)
{
    ui->setupUi(this);

    options = vsg::Options::create();
    options->fileCache = vsg::getEnv("RRS2_CACHE");
    options->paths = vsg::getEnvPaths("RRS2_ROOT");

    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());

    QSettings settings(app::ORGANIZATION_NAME, app::APP_NAME);
    auto HMH = settings.value("HMH", 1.0).toDouble();
    auto NFR = settings.value("NFR", 0.0001).toDouble();

    ui->HMHSpin->setValue(HMH);
    ui->NFRSpin->setValue(NFR);
    ui->pointsSpinBox->setValue(settings.value("POINTSIZE", 3).toInt());
    ui->lodPointsSpinBox->setValue(settings.value("LOD_POINTS", 0.1).toDouble());
    ui->lodTilesSpinBox->setValue(settings.value("LOD_TILES", 0.5).toDouble());
    ui->cursorSpinBox->setValue(settings.value("CURSORSIZE", 3).toInt());

    routeModel = new QFileSystemModel(this);
    ui->routeTree->setModel(routeModel);
    ui->routeTree->setRootIndex(routeModel->setRootPath(qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "routes"));
    auto skyfsmodel = new QFileSystemModel(this);
    ui->skyTree->setModel(skyfsmodel);
    ui->skyTree->setRootIndex(skyfsmodel->setRootPath(qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "sky"));
    connect(ui->skyTree->selectionModel(), &QItemSelectionModel::selectionChanged, [this, skyfsmodel](const QItemSelection &selected, const QItemSelection &)
    {
        skyboxPath = skyfsmodel->filePath(selected.indexes().front());
    });

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &StartDialog::load);
}
void StartDialog::updateSettings()
{
    QSettings settings(app::ORGANIZATION_NAME, app::APP_NAME);
    settings.setValue("HMH", ui->HMHSpin->value());
    settings.setValue("NFR", ui->NFRSpin->value());
    settings.setValue("COLORS", ui->comboBox->currentIndex());
    settings.setValue("POINTSIZE", ui->pointsSpinBox->value());
    settings.setValue("LOD_POINTS", ui->lodPointsSpinBox->value());
    settings.setValue("LOD_TILES", ui->lodTilesSpinBox->value());
    settings.setValue("CURSORSIZE", ui->cursorSpinBox->value());
}

void StartDialog::load()
{
    app::registerObjectFactoy();

    auto selected = ui->routeTree->selectionModel()->selectedRows();
    std::vector<vsg::ref_ptr<route::Tile>> loaded(selected.size());

    auto load = [this](const QModelIndex &idx)
    {
        auto path = routeModel->filePath(idx);
        auto node = vsg::read_cast<route::Tile>(path.toStdString(), options);
        if(!node)
            throw DatabaseException(path);
        node->terrain->properties.dataVariance = vsg::DYNAMIC_DATA;
        node->texture->properties.dataVariance = vsg::DYNAMIC_DATA;
        node->setValue(app::PATH, path.toStdString());
        return node;
    };
    std::transform(std::execution::par, selected.begin(), selected.end(), loaded.begin(), load);

    auto fi = routeModel->fileInfo(selected.front());
    auto databasePath = fi.absolutePath() + QDir::separator() + "database." + fi.suffix();
    auto route = vsg::read_cast<route::Route>(databasePath.toStdString(), options);
    if (!route)
        throw (DatabaseException(databasePath));
    route->setValue(app::PATH, databasePath.toStdString());
    for (const auto &ptr : loaded) {
        route->tiles->addChild(ptr);
    }
    database = DatabaseManager::create(route, options);
}

StartDialog::~StartDialog()
{   
    delete ui;
}

