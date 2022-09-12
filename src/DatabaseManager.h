#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneModel.h"
#include <QSettings>
#include <QProgressBar>
#include <QFileSystemModel>
#include <vsgXchange/all.h>
#include <QtConcurrent>
#include "SceneObjectVisitor.h"
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/Switch.h>

namespace route {
    class Topology;
}

class Manipulator;

class DatabaseException
{
public:
    DatabaseException(const QString &path)
        : err_path(path)
    {
    }
    QString getErrPath() { return err_path; }
private:
    QString err_path;
};

class PointsGroup : public vsg::Inherit<vsg::Group, PointsGroup>
{
public:
    PointsGroup() : vsg::Inherit<vsg::Group, PointsGroup>() {}

    void read(vsg::Input& input) override { vsg::Node::read(input); }
    void write(vsg::Output& output) const override { vsg::Node::write(output); }

protected:
    virtual ~PointsGroup(){}
};

class DatabaseManager : public vsg::Inherit<vsg::Object, DatabaseManager>
{
public:
    DatabaseManager(vsg::ref_ptr<vsg::Group> database, vsg::ref_ptr<vsg::Group> nodes, vsg::ref_ptr<vsg::Options> options);
    virtual ~DatabaseManager();

    void setUndoStack(QUndoStack *stack);
    void setViewer(vsg::ref_ptr<vsg::Viewer> viewer);

    vsg::ref_ptr<vsg::Group> getDatabase() const noexcept;
    QFuture<void> loadTiles(QProgressBar *bar = nullptr);
    vsg::ref_ptr<vsg::Node> getStdWireBox();
    vsg::ref_ptr<vsg::Node> getStdAxis();

    QUndoStack *undoStack;

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Viewer> viewer;

    vsg::ref_ptr<route::Topology> topology;
    vsg::ref_ptr<vsg::CopyAndReleaseImage> copyImageCmd;

    vsg::ref_ptr<vsg::Group> root;

    SceneModel *tilesModel;

    void writeTiles();

private:
    void compile();
    bool _compiled = false;

    vsg::ref_ptr<vsg::Group> _database;
    vsg::ref_ptr<vsg::Node> _stdWireBox;
    vsg::ref_ptr<vsg::Group> _stdAxis;
};

#endif // DATABASEMANAGER_H
