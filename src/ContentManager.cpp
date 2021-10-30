#include "ContentManager.h"
#include "sceneobjects.h"
#include "ui_ContentManager.h"
#include <QSettings>
#include <QtConcurrent/QtConcurrent>
#include "DatabaseManager.h"

ContentManager::ContentManager(vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Options> in_options, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContentManager),
    options(in_options),
    builder(in_builder)
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    ui->setupUi(this);
    model = new QFileSystemModel(this);
    model->setRootPath(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString());
    ui->fileView->setModel(model);
    ui->fileView->setRootIndex(model->index(settings.value("CONTENT", "/home/asafr/Development/vsg/vsgExamples/data").toString()));
    QDirIterator it(model->rootPath(), QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList models;
    while (it.hasNext())
        models << it.next();
    loadModels(models);
}

void addToGroup(QMap<QString, vsg::ref_ptr<vsg::Node>> &preloaded, std::pair<QString, vsg::ref_ptr<vsg::Node>> &node)
{
    preloaded.insert(node.first, node.second);
}

std::pair<QString, vsg::ref_ptr<vsg::Node>> read(const QString &path)
{
    auto tile = vsg::read_cast<vsg::Node>(path.toStdString());
    if (tile)
    {
        return std::make_pair(path, tile);
    } else
        throw (DatabaseException(path));
}

void ContentManager::loadModels(QStringList tileFiles)
{
    QFuture<QMap<QString, vsg::ref_ptr<vsg::Node>>> future = QtConcurrent::mappedReduced(tileFiles, &DatabaseManager::read, addToGroup, );
    future.waitForFinished();
}




ContentManager::~ContentManager()
{
    delete ui;
}
