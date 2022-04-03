#include "StageDialog.h"
#include "ui_StageDialog.h"

StageDialog::StageDialog(DatabaseManager *db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StageDialog)
{
    ui->setupUi(this);

    model = new StationsModel(db->topology);
    ui->beginList->setModel(model);
    ui->endList->setModel(model);
}

StageDialog::~StageDialog()
{
    delete ui;
}


void StageDialog::accept()
{
    auto beginSelected = ui->beginList->selectionModel()->selection().indexes();
    auto endSelected = ui->endList->selectionModel()->selection().indexes();
    if(beginSelected.isEmpty() || endSelected.isEmpty())
        return;

    auto beginSt = model->station(beginSelected.front());
    auto endSt = model->station(endSelected.front());
    if(!beginSt || !endSt)
        return;

    beginSt->stages.insert({endSt, route::Stage::create()});
}
