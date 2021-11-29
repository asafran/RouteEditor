#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <QObject>
#include <vsg/all.h>
#include "SceneModel.h"
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

class MouseHandler : public QObject
{
    Q_OBJECT
public:
    MouseHandler(vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer,
                QUndoStack *stack,
                SceneModel *model,
                QObject *parent = nullptr);
    ~MouseHandler();

    enum Mode
    {
        SELECT,
        MOVE,
        ADD,
        TERRAIN,
        ADDTRACK
    };

    void setPager(vsg::ref_ptr<vsg::DatabasePager> pager) { database = pager; }

public slots:
    void setMode(int index);
    void handleIntersection(vsg::LineSegmentIntersector::Intersections intersections);

signals:
    void addRequest(const vsg::dvec3& position, const QModelIndex &index);
    void objectClicked(const QModelIndex &index);
    void sendStatusText(const QString &message, int timeout);
    void addTrackRequest(const vsg::dvec3 &position, Trajectory *traj);

protected:
    vsg::ref_ptr<vsg::MatrixTransform> addTerrainPoint(vsg::vec3 pos);

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::DatabasePager> database;

    //---------------------Terrain---------------------
    vsg::ref_ptr<vsg::Group> terrainPoints;
    vsg::ref_ptr<vsg::MatrixTransform> active;
    vsg::MatrixTransform *movingPoint;
    vsg::ref_ptr<vsg::BufferInfo> info;
    vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBufferCmd;
    QMap<const vsg::Node*, vsg::stride_iterator<vsg::vec3>> points;
    //-------------------------------------------------

    //----------------------Move-----------------------
    SceneObject *movingObject;

    SceneModel *tilesModel;
    QUndoStack *undoStack;

    int mode = SELECT;
    bool isMovingTerrain = false;
    bool isMovingObject = false;
};

#endif // MOUSEHANDLER_H
