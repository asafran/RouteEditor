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
                QUndoStack *stack,
                SceneModel *model,
                QObject *parent = nullptr);
    ~Manipulator();

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
    void apply(vsg::PointerEvent& pointerEvent) override;

    void rotate(double angle, const vsg::dvec3& axis) override;
    void zoom(double ratio) override;
    void pan(const vsg::dvec2& delta) override;

    vsg::LineSegmentIntersector::Intersection interesection(vsg::PointerEvent& pointerEvent);
    enum Mode
    {
        SELECT,
        ADD,
        MOVE
    };

    void setPager(vsg::ref_ptr<vsg::DatabasePager> pager) { database = pager; }
    void setViewpoint(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);

public slots:
    void selectObject(const QModelIndex &index);
    void addAction(bool checked);

signals:
    void addRequest(const vsg::dvec3& position, const QModelIndex &index);
    void objectClicked(const QModelIndex &index);
    void expand(const QModelIndex &index);

protected:
    inline void addPointer();
    vsg::ref_ptr<vsg::Node> lowTile(const vsg::LineSegmentIntersector::Intersection &intersection);

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::DatabasePager> database;
    //vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::MatrixTransform> pointer;
    SceneModel *tilesModel;

    double height = 0.01;
    int mode = SELECT;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

#endif // MANIPULATOR_H
