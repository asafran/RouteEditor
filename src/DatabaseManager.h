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


namespace route {
    class Topology;
}


class Manipulator;
/*
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
*/

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
    DatabaseManager(QString path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QFileSystemModel *model, QObject *parent = nullptr);
    virtual ~DatabaseManager();

    vsg::ref_ptr<vsg::Node> getRoot() const noexcept { return root; }
    vsg::ref_ptr<vsg::Node> getDatabase() const noexcept { return database; }
    SceneModel *loadTiles(vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer, double tileLOD, double pointsLOD, float size);
    SceneModel *getTilesModel() noexcept { return tilesModel; }

    //vsg::ref_ptr<vsg::Node> read(const QString &path) const;


    enum ObjectType
    {
        Obj,
        Trk,
        TrkObj
    };
    struct Loaded
    {
        QString path = "";
        ObjectType type = Obj;
        vsg::ref_ptr<vsg::Node> node;
        explicit operator bool() { return node.valid(); }
    };

    void addToClicked(const FindNode &found) noexcept;

    void addTrack(const vsg::dvec3 &pos) noexcept;

public slots:
    void writeTiles() noexcept;

    //void addTrack(SceneTrajectory *traj, double position = 0.0) noexcept;
    void activeGroupChanged(const QModelIndex &index) noexcept;
    void activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept;
    void loaderButton(bool checked) noexcept;

signals:
    void sendStatusText(const QString &message, int timeout);

private:

    void addToTrack(route::Trajectory *to, double coord) noexcept;

    //vsg::ref_ptr<vsg::Node> loadSelected() const;

    //QPair<QString, vsg::ref_ptr<vsg::Node>> concurrentRead(const QString &path);

    //Trajectory *createTrajectory(const Loaded &loaded, Trajectory *prev = nullptr);

    //inline vsg::ref_ptr<vsg::MatrixTransform> addTerrainPoint(vsg::vec3 pos);

    vsg::ref_ptr<vsg::Group> root;
    vsg::ref_ptr<vsg::Group> database;
    //vsg::ref_ptr<vsg::Group> tiles;
    std::map<vsg::Node*, const QString> files;

    QString databasePath;

    vsg::ref_ptr<route::Topology> topology;

    vsg::ref_ptr<vsg::Builder> builder;

    QModelIndex activeGroup;

    QString loadedPath;
    vsg::ref_ptr<vsg::Node> loaded;

    QFileSystemModel *fsmodel;

    SceneModel *tilesModel;

    bool loadToSelected = false;

    bool placeLoader = false;

    QDir modelsDir;

    QUndoStack *undoStack;

    friend Manipulator;
};

#endif // DATABASEMANAGER_H
