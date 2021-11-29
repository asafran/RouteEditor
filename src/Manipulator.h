#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include "MouseHandler.h"

class Manipulator : public MouseHandler, public vsg::Inherit<vsg::Trackball, Manipulator>
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

    vsg::LineSegmentIntersector::Intersections interesection(vsg::PointerEvent& pointerEvent);

public slots:
    void selectObject(const QModelIndex &index);
    void setViewpoint(const vsg::dvec3 &pos);
    void setLatLongAlt(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);

signals:
    void expand(const QModelIndex &index);
    void sendData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);
    void sendPos(const vsg::dvec3 &pos);

protected:
    inline void addPointer();

    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::MatrixTransform> pointer;

    //----------------------Move-----------------------
    vsg::dmat4 oldMatrix;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

vsg::ref_ptr<vsg::Group> lowTile(const vsg::LineSegmentIntersector::Intersection &intersection, uint64_t frameCount);
#endif // MANIPULATOR_H
