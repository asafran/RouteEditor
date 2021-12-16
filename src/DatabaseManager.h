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

class Topology;

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
    DatabaseManager(const QString &path, QUndoStack *stack, vsg::ref_ptr<vsg::Builder> in_builder, QFileSystemModel *model, QObject *parent = nullptr);
    virtual ~DatabaseManager();

    vsg::ref_ptr<vsg::Node> getDatabase() const noexcept { return database; }
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

public slots:
    void writeTiles() noexcept;
    void addObject(vsg::dvec3 position, const QModelIndex &index) noexcept;
    void addTrack(SceneTrajectory *traj, double position = 0.0) noexcept;
    void activeGroupChanged(const QModelIndex &index) noexcept;
    void activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept;
    void loaderButton(bool checked) noexcept;
signals:
    void sendStatusText(const QString &message, int timeout);

private:

    QPair<QString, vsg::ref_ptr<vsg::Node>> concurrentRead(const QString &path);

    Trajectory *createTrajectory(const Loaded &loaded, Trajectory *prev = nullptr);

    vsg::ref_ptr<vsg::Group> database;
    std::string databasePath;

    vsg::ref_ptr<Topology> topology;

    vsg::ref_ptr<vsg::Builder> builder;

    QModelIndex activeGroup;

    Loaded loaded;

    QFileSystemModel *fsmodel;

    SceneModel *tilesModel;

    bool loadToSelected = false;

    bool placeLoader = false;

    QDir modelsDir;

    QUndoStack *undoStack;
};

#endif // DATABASEMANAGER_H
