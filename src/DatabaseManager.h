#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneModel.h"
#include <QSettings>
#include <QFileSystemModel>
#include <vsgXchange/all.h>
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

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    DatabaseManager(QString path, vsg::ref_ptr<vsg::Options> options, QObject *parent = nullptr);
    virtual ~DatabaseManager();

    vsg::ref_ptr<vsg::Group> getDatabase() const noexcept;
    void loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, vsg::ref_ptr<vsg::CopyAndReleaseImage> copyImage);
    void setUpBuilder(vsg::ref_ptr<vsg::Builder> in_builder);
    vsg::ref_ptr<vsg::Node> getStdWireBox();

    vsg::ref_ptr<vsg::CopyAndReleaseImage> getImageCmd() noexcept { return _copyImageCmd; }

    QUndoStack *undoStack;

    vsg::ref_ptr<vsg::Builder> builder;

    vsg::ref_ptr<route::Topology> topology;

    vsg::ref_ptr<vsg::Group> root;

    SceneModel *tilesModel;

public slots:
    void writeTiles() noexcept;

signals:
    void sendStatusText(const QString &message, int timeout);

private:
    void addPoints(const vsg::Node *tile, vsg::ref_ptr<vsg::Node> sphere, vsg::ref_ptr<vsg::Group> points);

    vsg::ref_ptr<vsg::Group> _database;
    std::map<vsg::Node*, const QString> _files;

    vsg::ref_ptr<vsg::Node> _stdWireBox;

    vsg::ref_ptr<vsg::CopyAndReleaseBuffer> _copyBufferCmd;
    vsg::ref_ptr<vsg::CopyAndReleaseImage> _copyImageCmd;
    QString _databasePath;
};

#endif // DATABASEMANAGER_H
