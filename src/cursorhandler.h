#ifndef CURSORHANDLER_H
#define CURSORHANDLER_H

#include <vsg/traversals/Intersector.h>
#include <vsg/utils/Builder.h>

class CursorHandler : public vsg::Inherit<vsg::Visitor, CursorHandler>
{
public:
    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Camera> camera;
    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel;
    double scale = 1.0;
    bool verbose = true;

    CursorHandler(vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Camera> in_camera, vsg::ref_ptr<vsg::Group> in_scenegraph, vsg::ref_ptr<vsg::EllipsoidModel> in_ellipsoidModel, double in_scale, vsg::ref_ptr<vsg::Options> in_options);

    void apply(vsg::KeyPressEvent& keyPress) override;

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

    void apply(vsg::PointerEvent& pointerEvent) override;

    void interesection(vsg::PointerEvent& pointerEvent);

protected:
    vsg::ref_ptr<vsg::PointerEvent> lastPointerEvent;
    vsg::LineSegmentIntersector::Intersection lastIntersection;
};

#endif // CURSORHANDLER_H
