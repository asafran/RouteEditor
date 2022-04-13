#include "SignalManager.h"
#include "sceneobjects.h"
#include "ui_SignalManager.h"
#include <QSettings>
#include <vsg/io/read.h>
#include <vsg/nodes/LOD.h>
#include <vsg/io/ObjectCache.h>
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include "DatabaseManager.h"

SignalManager::SignalManager(DatabaseManager *database, QString root, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::SignalManager)
  , modelsDir(root)
{
    ui->setupUi(this);
    _fsmodel = new QFileSystemModel(this);
    _fsmodel->setRootPath(root);
    _fsmodel->setNameFilters(app::FORMATS);
    _fsmodel->setNameFilterDisables(false);
    ui->fileView->setModel(_fsmodel);
    ui->fileView->setRootIndex(_fsmodel->index(root));
}

void SignalManager::intersection(const FoundNodes &isection)
{
    if(!isection.connector)
        return;
    if(ui->removeBox->isChecked())
    {
        if(ui->reverseBox->isChecked())
            _database->undoStack->push(new RemoveBwdSignal(isection.connector, _database->topology));
        else
            _database->undoStack->push(new RemoveFwdSignal(isection.connector, _database->topology));
        return;
    }
    if(ui->fileView->selectionModel()->selectedIndexes().isEmpty())
        return;
    auto activeFile = ui->fileView->selectionModel()->selectedIndexes().front();
    if(!activeFile.isValid())
        return;

    auto path = _fsmodel->filePath(activeFile).toStdString();
    _database->builder->options->objectCache->remove(path);
    auto node = vsg::read_cast<vsg::Node>(path, _database->builder->options);
    if(!node)
        return;

    if(ui->reverseBox->isChecked())
    {
        auto flip = vsg::MatrixTransform::create(vsg::rotate(vsg::PI, vsg::dvec3(0.0, 0.0, 1.0)));
        flip->addChild(node);
        node = flip;
    }

    auto lod = vsg::LOD::create();

    vsg::LOD::Child hires{0.01, node};
    vsg::LOD::Child dummy{0.0, vsg::Node::create()};
    lod->addChild(hires);
    lod->addChild(dummy);

    auto box = vsg::visit<vsg::ComputeBounds>(node).bounds;
    lod->bound.center = (box.min + box.max) * 0.5;
    lod->bound.radius = length(box.max - box.min) * 0.5;

    _database->builder->compileTraversal->compile(node);

    vsg::ref_ptr<signalling::Signal> sig;

    bool fstate = ui->fstateBox->isChecked();

    try {
        switch (ui->typeBox->currentIndex()) {
        case Auto:
            sig = signalling::AutoBlockSignal::create(lod, _database->getStdWireBox(), fstate);
            break;
        case Routing:
            sig = signalling::RouteSignal::create(lod, _database->getStdWireBox(), fstate);
            break;
        }
    }  catch (signalling::SigException e) {
        emit sendStatusText(tr("Отсутствует сигнал %1").arg(e.errL), 2000);
    }

    sig->recalculateWireframe();

    if(!isection.connector->fwdSignal() && !ui->reverseBox->isChecked())
        _database->undoStack->push(new AddFwdSignal(isection.connector, sig, _database->topology));
    else if(!isection.connector->bwdSignal() && ui->reverseBox->isChecked())
        _database->undoStack->push(new AddBwdSignal(isection.connector, sig, _database->topology));

    emit sendStatusText(tr("Добавлен объект %1").arg(path.c_str()), 2000);
}

SignalManager::~SignalManager()
{
    delete ui;
}
