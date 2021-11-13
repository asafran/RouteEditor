#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <QObject>
#include <vsg/all.h>
#include "SceneModel.h"
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::Group> in_scenegraph,
                vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer,
                QUndoStack *stack,
                SceneModel *model,
                QObject *parent = nullptr);
    ~Manipulator();

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
    void apply(vsg::MoveEvent& pointerEvent) override;

    void rotate(double angle, const vsg::dvec3& axis) override;
    void zoom(double ratio) override;
    void pan(const vsg::dvec2& delta) override;

    vsg::LineSegmentIntersector::Intersection interesection(vsg::PointerEvent& pointerEvent);
    enum Mode
    {
        SELECT,
        MOVE,
        ADD,
        TERRAIN
    };

    void setPager(vsg::ref_ptr<vsg::DatabasePager> pager) { database = pager; }
    void setViewpoint(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);

public slots:
    void selectObject(const QModelIndex &index);
    void setMode(int index);

signals:
    void addRequest(const vsg::dvec3& position, const QModelIndex &index);
    void objectClicked(const QModelIndex &index);
    void expand(const QModelIndex &index);
    void sendData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);

protected:
    inline void addPointer();
    vsg::ref_ptr<vsg::MatrixTransform> addTerrainPoint(vsg::vec3 pos);
    vsg::ref_ptr<vsg::Group> lowTile(const vsg::LineSegmentIntersector::Intersection &intersection);

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::DatabasePager> database;
    //vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::MatrixTransform> pointer;

    //---------------------Terrain---------------------
    vsg::ref_ptr<vsg::BufferInfo> info;
    vsg::ref_ptr<vsg::Group> terrainPoints;
    vsg::ref_ptr<vsg::MatrixTransform> active;
    vsg::MatrixTransform *moving;
    vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBufferCmd;
    QMap<const vsg::Node*, vsg::stride_iterator<vsg::vec3>> points;
    //-------------------------------------------------

    //----------------------Move-----------------------
    SceneObject *movingObject;
    vsg::dmat4 oldMatrix;

    SceneModel *tilesModel;
    QUndoStack *undoStack;

    double height = 0.01;
    int mode = SELECT;
    bool isMovingTerrain = false;
    bool isMovingObject = false;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

#endif // MANIPULATOR_H
