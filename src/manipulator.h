#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <QObject>
#include <vsg/all.h>

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::Group> in_scenegraph,
                double in_scale,
                vsg::ref_ptr<vsg::Options> in_options, QObject *parent = nullptr);

    void apply(vsg::KeyPressEvent& keyPress) override;

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

    void apply(vsg::PointerEvent& pointerEvent) override;

    vsg::LineSegmentIntersector::Intersection interesection(vsg::PointerEvent& pointerEvent);

signals:
    void tileClicked();

protected:

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Group> scenegraph;
    double height = 0.01;
    bool verbose = false;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

#endif // MANIPULATOR_H
