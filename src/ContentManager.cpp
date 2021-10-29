#include "ContentManager.h"
#include "sceneobjects.h"
#include "ui_ContentManager.h"
#include <QSettings>

ContentManager::ContentManager(vsg::ref_ptr<vsg::Builder> builder, vsg::ref_ptr<vsg::Options> options, SceneModel *model, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContentManager),
    _options(options),
    _builder(builder),
    _tilesModel(model)
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    ui->setupUi(this);
    _model = new QFileSystemModel(this);
    _model->setRootPath(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString());
    ui->fileView->setModel(_model);
    ui->fileView->setRootIndex(_model->index(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString()));


}
void ContentManager::addObject(const vsg::LineSegmentIntersector::Intersection &isection, const QModelIndex &index)
{
    vsg::ref_ptr<vsg::Group> group;
    auto readindex = index.isValid() ? index : _active;
    if(readindex.isValid())
        group = static_cast<vsg::Node*>(index.internalPointer())->cast<vsg::Group>();
    if (ui->fileView->selectionModel()->selectedIndexes().isEmpty() || !group)
        return;
    const QFileInfo info(_model->fileInfo(ui->fileView->selectionModel()->selectedIndexes().front()));
    if(auto node = vsg::read_cast<vsg::Node>(info.canonicalFilePath().toStdString(), _options); node)
    {
        _builder->compile(node);
        auto obj = SceneObject::create(node, isection.localToWord, info.fileName().toStdString(), vsg::translate(isection.localIntersection));
        _tilesModel->addNode(readindex, new AddNode(group.get(), obj));
    }
}
void ContentManager::setActiveGroup(const QItemSelection &selected, const QItemSelection &)
{
    _active = selected.indexes().front();
}

ContentManager::~ContentManager()
{
    delete ui;
}
