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

    ui->HMHedit->setText(QString::number(HMH));
    ui->NFRedit->setText(QString::number(NFR));

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
    settings.setValue("HMH", ui->HMHedit->text().toDouble());
    settings.setValue("NFR", ui->NFRedit->text().toDouble());
    settings.setValue("COLORS", ui->comboBox->currentIndex());
    settings.setValue("CURSORSIZE", ui->cursorSpinBox->value());
}

StartDialog::~StartDialog()
{   
    delete ui;
}

