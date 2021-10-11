#include "ContentManager.h"
#include "ui_ContentManager.h"
#include <QSettings>

ContentManager::ContentManager(vsg::ref_ptr<vsg::Options> options, QUndoStack *stack, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContentManager),
    _options(options),
    _stack(stack)
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    ui->setupUi(this);
    model = new QFileSystemModel(this);
    model->setRootPath(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString());
    ui->fileView->setModel(model);
    ui->fileView->setRootIndex(model->index(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString()));

}
void ContentManager::addObject(const vsg::dvec3 &pos)
{
    if (ui->fileView->selectionModel()->selectedIndexes().isEmpty())
        return;
    const QFileInfo info(model->fileInfo(ui->fileView->selectionModel()->selectedIndexes().front()));
    if(auto node = vsg::read_cast<vsg::Node>(info.absoluteFilePath().toStdString(), _options); node && _active)
    {
        auto matrix = vsg::MatrixTransform::create(vsg::translate(pos));
        matrix->addChild(node);
        //_stack->beginMacro(tr("Новый объект"));
        _stack->push(new AddNode(_active.get(), matrix.get()));
        //_stack->push(new MoveObject(object, vsg::translate(pos)));
        //_stack->endMacro();
    }
}
void ContentManager::setActiveGroup(const QItemSelection &selected, const QItemSelection &deselected)
{
    if(!selected.indexes().front().isValid())
        return;
    _active.release();
    if(auto group = static_cast<vsg::Node*>(selected.indexes().front().internalPointer())->cast<vsg::Group>(); group)
        _active = group;
}

ContentManager::~ContentManager()
{
    delete ui;
}
