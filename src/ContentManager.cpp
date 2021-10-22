#include "ContentManager.h"
#include "sceneobjects.h"
#include "ui_ContentManager.h"
#include <QSettings>

ContentManager::ContentManager(vsg::ref_ptr<vsg::Builder> builder, vsg::ref_ptr<vsg::Options> options, QUndoStack *stack, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContentManager),
    _options(options),
    _builder(builder),
    _stack(stack)
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    ui->setupUi(this);
    model = new QFileSystemModel(this);
    model->setRootPath(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString());
    ui->fileView->setModel(model);
    ui->fileView->setRootIndex(model->index(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString()));


}
void ContentManager::addObject(vsg::LineSegmentIntersector::Intersection isection)
{
    if (ui->fileView->selectionModel()->selectedIndexes().isEmpty() || !_active)
        return;
    const QFileInfo info(model->fileInfo(ui->fileView->selectionModel()->selectedIndexes().front()));
    if(auto node = vsg::read_cast<vsg::Node>(info.canonicalFilePath().toStdString(), _options); node)
    {
        _builder->compile(node);
        auto obj = SceneObject::create(node, info, vsg::translate(isection.localIntersection));
        _stack->push(new AddNode(_active.get(), obj));
    }
}
void ContentManager::setActiveGroup(const QItemSelection &selected, const QItemSelection &deselected)
{
    if(selected.empty() || !selected.indexes().front().isValid())
        return;
    _active.release();
    if(auto group = static_cast<vsg::Node*>(selected.indexes().front().internalPointer())->cast<vsg::Group>(); group != nullptr)
        _active = group;
}

ContentManager::~ContentManager()
{
    delete ui;
}
