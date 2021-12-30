#include "StartDialog.h"
#include "ui_StartDialog.h"
#include <QFileSystemModel>
#include <QSettings>

StartDialog::StartDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartDialog)
{
    ui->setupUi(this);

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    auto HMH = settings.value("HMH", 1.0).toDouble();
    auto NFR = settings.value("NFR", 0.0001).toDouble();

    ui->HMHSpin->setValue(HMH);
    ui->NFRSpin->setValue(NFR);
    ui->pointsSpinBox->setValue(settings.value("POINTSIZE", 3).toInt());
    ui->lodPointsSpinBox->setValue(settings.value("LOD_POINTS", 0.1).toDouble());
    ui->lodTilesSpinBox->setValue(settings.value("LOD_TILES", 0.5).toDouble());
    ui->cursorSpinBox->setValue(settings.value("CURSORSIZE", 3).toInt());

    auto routefsmodel = new QFileSystemModel(this);
    ui->routeTree->setModel(routefsmodel);
    ui->routeTree->setRootIndex(routefsmodel->setRootPath(qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "routes"));
    auto skyfsmodel = new QFileSystemModel(this);
    ui->skyTree->setModel(skyfsmodel);
    ui->skyTree->setRootIndex(skyfsmodel->setRootPath(qgetenv("RRS2_ROOT") + QDir::separator().toLatin1() + "sky"));
    connect(ui->routeTree->selectionModel(), &QItemSelectionModel::selectionChanged, [this, routefsmodel](const QItemSelection &selected, const QItemSelection &)
    {
        routePath = routefsmodel->filePath(selected.indexes().front());
    });
    connect(ui->skyTree->selectionModel(), &QItemSelectionModel::selectionChanged, [this, skyfsmodel](const QItemSelection &selected, const QItemSelection &)
    {
        skyboxPath = skyfsmodel->filePath(selected.indexes().front());
    });
}
void StartDialog::updateSettings()
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    settings.setValue("HMH", ui->HMHSpin->value());
    settings.setValue("NFR", ui->NFRSpin->value());
    settings.setValue("COLORS", ui->comboBox->currentIndex());
    settings.setValue("POINTSIZE", ui->pointsSpinBox->value());
    settings.setValue("LOD_POINTS", ui->lodPointsSpinBox->value());
    settings.setValue("LOD_TILES", ui->lodTilesSpinBox->value());
    settings.setValue("CURSORSIZE", ui->cursorSpinBox->value());
}

StartDialog::~StartDialog()
{   
    delete ui;
}

