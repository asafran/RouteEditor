#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneModel.h"
#include <QSettings>
#include <vsgXchange/all.h>

class TilesVisitor : public vsg::ConstVisitor
{
public:
    TilesVisitor(vsg::ref_ptr<vsg::Group> group) :
    tiles(group)
    {
    }

    using vsg::ConstVisitor::apply;

    void apply(const vsg::Object& object) override
    {
            object.traverse(*this);
    }

    void apply(const vsg::PagedLOD& plod) override
    {
        if(auto group = plod.children.front().node.cast<vsg::Group>(); group
                && group->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
        {
            QFileInfo file(plod.filename.c_str());
            group->setValue(META_NAME, file.absoluteFilePath().toStdString());
            tiles->addChild(group);
            filenames.insert(file.absoluteFilePath());
        }
        plod.traverse(*this);
    }
    vsg::ref_ptr<vsg::Group> tiles;
    QSet<QString> filenames;
};

class DatabaseException : public QException
{
public:
    DatabaseException(const QString &path)
        : err_path(path)
    {
    }
    void raise() const override { throw *this; }
    DatabaseException *clone() const override { return new DatabaseException(*this); }
    QString getErrPath() { return err_path; }
private:
    QString err_path;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &path, QUndoStack *stack, QObject *parent = nullptr);

    vsg::ref_ptr<vsg::Node> getDatabase() { return database; }
    SceneModel *getCahedTilesModel() { return cachedTilesModel; }
    SceneModel *loadTiles();


    static vsg::Node *read(const QString &path);

public slots:
    void updateTileCache();
    void writeTiles();

signals:
    void updateViews();

private:
    vsg::ref_ptr<vsg::Group> cachedTiles;
    vsg::ref_ptr<vsg::Node> database;
    QSet<QString> tileFiles;
    SceneModel *fileTilesModel;
    SceneModel *cachedTilesModel;
    QUndoStack *undoStack;
    QFileSystemWatcher *fsWatcher;
};

#endif // DATABASEMANAGER_H
