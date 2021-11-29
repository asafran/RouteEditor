#include "TrajectoryDialog.h"
#include "ui_TrajectoryDialog.h"

TrajectoryDialog::TrajectoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrajectoryDialog)
{
    ui->setupUi(this);
}

TrajectoryDialog::~TrajectoryDialog()
{
    delete ui;
}
