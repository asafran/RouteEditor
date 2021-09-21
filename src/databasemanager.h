#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <vsg/all.h>
#include <QDirIterator>
#include "SceneModel.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &path, vsg::Options *opt, QObject *parent = nullptr);

    vsg::Node* loadDatabase();
    SceneModel *getTilesModel();

public slots:


signals:
    void emitTilesRoot(const QModelIndex &index);

private:
    void cacheTiles();
    vsg::ref_ptr<vsg::Group> root;
    vsg::ref_ptr<vsg::Group> tiles;
    vsg::ref_ptr<vsg::Options> options;
    const QString path;
    SceneModel *tilesmodel;

};

#endif // DATABASEMANAGER_H
