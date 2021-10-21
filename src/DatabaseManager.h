#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneModel.h"
#include <QSettings>
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
    explicit DatabaseManager(const QString &path, QUndoStack *stack, SceneModel *model, QObject *parent = nullptr);

    vsg::ref_ptr<vsg::Node> getDatabase() { return database; }
    SceneModel *loadTiles();

    void setPager(vsg::ref_ptr<vsg::DatabasePager> in_pager) { pager = in_pager; }

    static vsg::Node *read(const QString &path);

public slots:
    void writeTiles();
    void updateTileCache();

private:
    vsg::ref_ptr<vsg::Node> database;
    vsg::ref_ptr<vsg::DatabasePager> pager;

    QSet<QString> tileFiles;
    QSet<QString> culledFiles;

    SceneModel *fileTilesModel;
    SceneModel *cachedTilesModel;

    uint32_t prevAvCount = 4000;

    QUndoStack *undoStack;
    QFileSystemWatcher *fsWatcher;
};

#endif // DATABASEMANAGER_H
