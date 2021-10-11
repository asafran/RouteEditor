#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QException>
#include "SceneModel.h"
#include "TilesVisitor.h"
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
    explicit DatabaseManager(const QString &path, QUndoStack *stack, QObject *parent = nullptr);

    vsg::ref_ptr<vsg::Node> getDatabase() { return database; }
    SceneModel *loadTiles();


    static vsg::Node *read(const QString &path);

public slots:
    void writeTiles(vsg::ref_ptr<vsg::Group> tiles);

private:
    vsg::ref_ptr<vsg::Node> database;
    QSet<QString> tileFiles;
    SceneModel *fileTilesModel;

    QUndoStack *undoStack;
    QFileSystemWatcher *fsWatcher;
};

#endif // DATABASEMANAGER_H
