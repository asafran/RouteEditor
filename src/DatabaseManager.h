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
#include "Compiler.h"
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
    DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> builder, QObject *parent = nullptr);
    virtual ~DatabaseManager();

    vsg::ref_ptr<vsg::Group> getRoot() const noexcept { return _root; }
    vsg::ref_ptr<vsg::Builder> getBuilder() const noexcept { return _builder; }
    //vsg::ref_ptr<Compiler> getCompiler() const noexcept { return _compiler; }
    void compile(vsg::ref_ptr<vsg::Node> subgraph) { _builder->compile(subgraph); }
    void push(QUndoCommand *cmd) { _undoStack->push(cmd); }
    vsg::ref_ptr<vsg::Group> getDatabase() const noexcept { return _database; }
    SceneModel *loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, vsg::ref_ptr<vsg::CopyAndReleaseImage> copyImage);
    SceneModel *getTilesModel() noexcept { return _tilesModel; }
    QUndoStack *getUndoStack() noexcept { return _undoStack; }
    vsg::ref_ptr<vsg::CopyAndReleaseImage> getImageCmd() noexcept { return _copyImageCmd; }

public slots:
    void writeTiles() noexcept;

signals:
    void sendStatusText(const QString &message, int timeout);

private:
    void addPoints(const vsg::Node *tile, vsg::ref_ptr<vsg::Node> sphere, vsg::ref_ptr<vsg::Group> points);

    vsg::ref_ptr<vsg::Builder> _builder;
    //vsg::ref_ptr<Compiler> _compiler;

    vsg::ref_ptr<vsg::Group> _root;
    vsg::ref_ptr<vsg::Group> _database;
    //vsg::ref_ptr<vsg::Group> tiles;
    std::map<vsg::Node*, const QString> _files;

    vsg::ref_ptr<vsg::CopyAndReleaseBuffer> _copyBufferCmd;
    vsg::ref_ptr<vsg::CopyAndReleaseImage> _copyImageCmd;
    QString _databasePath;

    vsg::ref_ptr<route::Topology> _topology;

    SceneModel *_tilesModel;

    QUndoStack *_undoStack;
};

#endif // DATABASEMANAGER_H
