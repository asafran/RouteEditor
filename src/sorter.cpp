#include "sorter.h"
#include "ui_sorter.h"

Sorter::Sorter(SceneModel *tilesmodel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Sorter)
  , model(tilesmodel)
{
    ui->setupUi(this);
    sorter.reset(new TilesSorter());
    sorter->setSourceModel(model.get());
    sorter->setFilterKeyColumn(1);
    sorter->setFilterWildcard("*");
    ui->treeView->setModel(sorter.get());
    connect(ui->lineEdit, &QLineEdit::textChanged, sorter.get(), &TilesSorter::setFilterWildcard);
    connect(ui->treeView, &QTreeView::doubleClicked, this, &Sorter::selected);
    connect(ui->treeView, &QTreeView::clicked, this, &Sorter::selected);
    connect(ui->treeView, &QTreeView::doubleClicked, this, &QDialog::close);
}

Sorter::~Sorter()
{
    delete ui;
}
