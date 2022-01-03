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


class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QObject *parent = nullptr);
    virtual ~DatabaseManager();

    vsg::ref_ptr<vsg::Group> getRoot() const noexcept { return _root; }
    vsg::ref_ptr<vsg::Builder> getBuilder() const noexcept { return _builder; }
    void compile(vsg::ref_ptr<vsg::Node> subgraph) { _builder->compile(subgraph); }
    void push(QUndoCommand *cmd) { _undoStack->push(cmd); }
    vsg::ref_ptr<vsg::Group> getDatabase() const noexcept { return _database; }
    SceneModel *loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, double tileLOD, double pointsLOD, float size);
    SceneModel *getTilesModel() noexcept { return _tilesModel; }

public slots:
    void writeTiles() noexcept;

signals:
    void sendStatusText(const QString &message, int timeout);

private:
    vsg::ref_ptr<vsg::Builder> _builder;

    vsg::ref_ptr<vsg::Group> _root;
    vsg::ref_ptr<vsg::Group> _database;
    //vsg::ref_ptr<vsg::Group> tiles;
    std::map<vsg::Node*, const QString> _files;

    QString _databasePath;

    vsg::ref_ptr<route::Topology> _topology;

    SceneModel *_tilesModel;

    QUndoStack *_undoStack;
};

#endif // DATABASEMANAGER_H
