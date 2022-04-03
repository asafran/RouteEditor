#include "InterlockDialog.h"
#include "ui_InterlockDialog.h"

InterlockDialog::InterlockDialog(DatabaseManager *db, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InterlockDialog)
    , _database(db)
{
    ui->setupUi(this);

    _sorter = new TilesSorter(this);
    //sorter->setSourceModel();
    _sorter->setFilterKeyColumn(1);
    _sorter->setFilterWildcard("*");
    ui->trackView->setModel(_sorter);

    ui->endList->setEnabled(false);
    ui->cmdList->setEnabled(false);

    connect(ui->searchLine, &QLineEdit::textChanged, _sorter, &TilesSorter::setFilterWildcard);
    /*connect(ui->tilesView->selectionModel(), &QItemSelectionModel::selectionChanged, sorter, &TilesSorter::viewSelectSlot);
    connect(ui->tilesView, &QTreeView::doubleClicked, sorter, &TilesSorter::viewDoubleClicked);
    connect(sorter, &TilesSorter::viewSelectSignal, ui->tilesView->selectionModel(),
             qOverload<const QModelIndex &, QItemSelectionModel::SelectionFlags>(&QItemSelectionModel::select));
    connect(sorter, &TilesSorter::viewExpandSignal, ui->tilesView, &QTreeView::expand);*/

    ui->stationBox->setModel(new StationsModel(_database->topology));

    connect(ui->stationBox, &QComboBox::currentIndexChanged, this, [this](int idx)
    {
        if(idx == -1)
        {
            _station = nullptr;
            return;
        }
        _station = std::next(_database->topology->stations.begin(), idx)->second;
        ui->beginList->selectionModel()->clear();
        ui->sigView->selectionModel()->clear();
        ui->endList->setEnabled(false);
        ui->cmdList->setEnabled(false);
        auto m = ui->beginList->model();
        auto nm = new RouteBeginModel(_station);
        ui->beginList->setModel(nm);
        ui->sigView->setModel(nm);
        delete m;
    });

    connect(ui->beginList, &QListView::clicked, this, [this](const QModelIndex &index)
    {
        if(!_station || !index.isValid())
        {
            _begin = nullptr;
            return;
        }
        _begin = std::next(_station->rsignals.begin(), index.row())->second;
        auto m = ui->endList->model();
        ui->endList->setModel(new RouteEndModel(_begin));
        ui->endList->setEnabled(true);
        delete m;
    });

    connect(ui->endList, &QListView::clicked, this, [this](const QModelIndex &index)
    {
        if(!_begin || !index.isValid())
        {
            _route = nullptr;
            return;
        }
        _route = std::next(_begin->routes.begin(), index.row())->second;
        auto m = ui->cmdList->model();
        ui->cmdList->setModel(new RouteCmdModel(_route));
        ui->cmdList->setEnabled(true);
        delete m;
    });

    connect(ui->addRouteButt, &QPushButton::pressed, this, &InterlockDialog::addRoute);
    connect(ui->addJButt, &QPushButton::pressed, this, &InterlockDialog::addJcts);
    connect(ui->addTrajsButt, &QPushButton::pressed, this, &InterlockDialog::addTrajs);
    connect(ui->addSigButt, &QPushButton::pressed, this, &InterlockDialog::addSignal);
}

InterlockDialog::~InterlockDialog()
{
    delete ui;
}

void InterlockDialog::addTrajs()
{
    if(!_route)
    {
        return;
    }

    auto selected = _sorter->mapSelectionToSource(ui->trackView->selectionModel()->selection()).indexes();
    if(selected.empty())
    {
        return;
    }

    for (const auto &index : selected)
    {
        auto node = static_cast<vsg::Node*>(index.internalPointer());
        if(auto trj = node->cast<route::Trajectory>(); trj)
            _route->trajs.emplace_back(trj);
    }
}

void InterlockDialog::addJcts()
{
    if(!_route)
    {
        return;
    }

    auto selected = _sorter->mapSelectionToSource(ui->trackView->selectionModel()->selection()).indexes();
    if(selected.empty())
    {
        return;
    }
    auto index = selected.front();
    auto node = static_cast<vsg::Node*>(index.internalPointer());
    if(auto j = node->cast<route::Junction>(); j)
        _route->commands.push_back(route::JunctionCommand::create(j, ui->sideBox->isChecked()));
}

void InterlockDialog::addSignal()
{
    if(!_station || !_route)
    {
        return;
    }

    auto selected = ui->sigView->selectionModel()->selectedIndexes();
    if(selected.empty())
    {
        return;
    }

    auto row = selected.front().row();
    auto signal = std::next(_station->rsignals.begin(), row);

    if(auto st = signal->first->cast<route::StSignal>(); st)
        _route->commands.push_back(route::SignalCommand::create(st, static_cast<route::StSignal::FwdHint>(ui->sigCombo->currentIndex())));
    else
    {

    }
}

void InterlockDialog::addRoute()
{
    if(!_station || !_begin)
    {
        return;
    }

    auto selected = ui->sigView->selectionModel()->selectedIndexes();
    if(selected.empty())
    {
        return;
    }

    auto row = selected.front().row();
    auto signal = std::next(_station->rsignals.begin(), row);

    if(_begin == signal->second)
    {
        return;
    }

    _route = route::Route::create();
    _begin->routes.insert({signal->first, _route});
    auto m = ui->endList->model();
    ui->endList->setModel(new RouteEndModel(_begin));
    ui->endList->setEnabled(true);
    delete m;

    auto mr = ui->cmdList->model();
    ui->cmdList->setModel(new RouteCmdModel(_route));
    ui->cmdList->setEnabled(true);
    delete mr;
}
