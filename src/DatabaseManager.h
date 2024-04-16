#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneObjectsModel.h"
#include "route.h"
#include <QSettings>
#include <QProgressBar>
#include <QFileSystemModel>
#include <vsg/app/EllipsoidModel.h>
#include <vsgXchange/all.h>
#include <QtConcurrent>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/Switch.h>
#include <vsg/threading/OperationThreads.h>

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

class DatabaseManager : public vsg::Inherit<vsg::Object, DatabaseManager>
{
public:
    DatabaseManager(vsg::ref_ptr<route::Route> in_route, vsg::ref_ptr<vsg::Options> options);
    virtual ~DatabaseManager();

    void setUndoStack(QUndoStack *stack);
    void setViewer(vsg::ref_ptr<vsg::Viewer> viewer);

    vsg::ref_ptr<vsg::Node> getStdWireBox();
    vsg::ref_ptr<vsg::Node> getStdAxis();

    QUndoStack *undoStack;

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Viewer> viewer;

    vsg::ref_ptr<vsg::OperationThreads> opThreads;

    vsg::ref_ptr<route::Route> route;
    vsg::ref_ptr<vsg::Group> root;

    vsg::ref_ptr<vsg::LookAt> lastLookAt;

    SceneModel *tilesModel;

    void writeTiles();

private:
    void compile();
    bool _compiled = false;


    vsg::ref_ptr<vsg::Node> _stdWireBox;
    vsg::ref_ptr<vsg::Group> _stdAxis;
};

#endif // DATABASEMANAGER_H
