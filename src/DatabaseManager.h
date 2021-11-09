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

    void setPager(vsg::ref_ptr<vsg::DatabasePager> in_pager) noexcept { pager = in_pager; }

    static vsg::Node *read(const QString &path);

public slots:
    void writeTiles() noexcept;
    void addObject(const vsg::dvec3& position, const QModelIndex &index) noexcept;
    void activeGroupChanged(const QModelIndex &index) noexcept;
    void activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept;
    void loaderAction(bool checked) noexcept;

private:
    vsg::ref_ptr<vsg::Node> database;
    vsg::ref_ptr<vsg::DatabasePager> pager;


    vsg::ref_ptr<vsg::Builder> builder;

    QModelIndex activeGroup;
    std::pair<QString, vsg::ref_ptr<vsg::Node>> activeFile;

    QFileSystemModel *fsmodel;

    SceneModel *tilesModel;

    bool placeLoader = false;

    QDir modelsDir;

    QUndoStack *undoStack;
    QFileSystemWatcher *fsWatcher;
};

#endif // DATABASEMANAGER_H
