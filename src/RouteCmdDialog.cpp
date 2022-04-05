#include "RouteCmdDialog.h"
#include "ui_RouteCmdDialog.h"

RouteCmdDialog::RouteCmdDialog(DatabaseManager *db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RouteCmdDialog)
{
    ui->setupUi(this);

    ui->endList->setEnabled(false);
    ui->beginList->setEnabled(false);

    auto bmodel = new RouteBeginModel(this);
    auto emodel = new RouteEndModel(this);
    ui->beginList->setModel(bmodel);
    ui->endList->setModel(emodel);

    ui->stationBox->setModel(new StationsModel(db->topology));
    ui->stationBox->setCurrentIndex(0);

    connect(ui->stationBox, &QComboBox::currentIndexChanged, this, [this, bmodel, db](int idx)
    {
        if(idx == -1)
        {
            station = nullptr;
            return;
        }
        station = std::next(db->topology->stations.begin(), idx)->second;
        ui->beginList->selectionModel()->clear();
        ui->beginList->setEnabled(true);
        ui->endList->setEnabled(false);
        bmodel->setStation(station);
    });

    connect(ui->beginList, &QListView::clicked, this, [this, emodel](const QModelIndex &index)
    {
        if(!station || !index.isValid())
        {
            begin = nullptr;
            return;
        }
        begin = std::next(station->rsignals.begin(), index.row())->second;
        emodel->setRoutes(begin);
        ui->endList->setEnabled(true);
    });

    connect(ui->endList, &QListView::clicked, this, [this](const QModelIndex &index)
    {
        if(!begin || !index.isValid())
        {
            route = nullptr;
            return;
        }
        route = std::next(begin->routes.begin(), index.row())->second;
    });
}

RouteCmdDialog::~RouteCmdDialog()
{
    delete ui;
}
