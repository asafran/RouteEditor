#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <QObject>
#include <vsg/all.h>
#include "SceneModel.h"
#include <QItemSelectionModel>

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::Group> in_scenegraph,
                QUndoStack *stack,
                vsg::ref_ptr<vsg::Options> in_options,
                SceneModel *model,
                QObject *parent = nullptr);
    ~Manipulator();

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

    void apply(vsg::PointerEvent& pointerEvent) override;

    vsg::LineSegmentIntersector::Intersection interesection(vsg::PointerEvent& pointerEvent);
    enum Mode
    {
        SELECT,
        ADD,
        MOVE
    };

    void setPager(vsg::ref_ptr<vsg::DatabasePager> pager) { database = pager; }

public slots:
    void selectObject(const QItemSelection &selected, const QItemSelection &);
    void setViewpoint(const vsg::dvec3 &pos);

signals:
    void addRequest(const vsg::dvec3 &pos);
    void objectClicked(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
    void expand(const QModelIndex &index);
    void updateCache();

protected:
    inline void addPointer();
    vsg::ref_ptr<vsg::Node> lowTile(const vsg::LineSegmentIntersector::Intersection &intersection);

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::DatabasePager> database;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::MatrixTransform> pointer;
    SceneModel *cachedTilesModel;

    double height = 0.01;
    int mode = ADD;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

#endif // MANIPULATOR_H
