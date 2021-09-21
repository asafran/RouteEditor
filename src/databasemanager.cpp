#include "databasemanager.h"

DatabaseManager::DatabaseManager(const QString &path, vsg::Options *opt, QObject *parent) : QObject(parent)
  , options(opt)
  , path(path)
  , tiles(vsg::Group::create())
{    
    tilesmodel = new SceneModel(tiles, this);
}

void DatabaseManager::cacheTiles()
{
    QString fileName = "L8";
    QStringList filter;
    if (!fileName.isEmpty())
        filter << fileName;
    QDirIterator it(path, filter, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList dirs;
    while (it.hasNext())
        dirs << it.next();

    std::function<vsg::Node> *load(const QString &path)
    {
        auto tile = vsg::read_cast<vsg::Node>(path.toStdString(), options);
        if (database)
        {
            root = database;
        }
    }
    if (!text.isEmpty())
        files = findFiles(files, text);
    files.sort();
    showFiles(files);
}

vsg::Node* DatabaseManager::loadDatabase()
{
    auto database = vsg::read_cast<vsg::Node>(path.toStdString(), options);
    if (database)
    {
        root = database;
        return database;
    }
    return nullptr;
}
