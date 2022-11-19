#ifndef INTERSECTIONHANDLER_H
#define INTERSECTIONHANDLER_H

#include <vsg/traversals/LineSegmentIntersector.h>
#include <vsg/utils/Builder.h>
#include <vsg/io/Options.h>

#include "animation.h"
#include "AnimationModel.h"

#include "QWidget"

namespace Ui {
class IntersectionHandler;
}

class IntersectionHandler : public QWidget, public vsg::Inherit<vsg::Visitor, IntersectionHandler>
{
public:
    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Camera> camera;

    IntersectionHandler(vsg::ref_ptr<vsg::Group> scenegraph, vsg::ref_ptr<AnimatedModel> model, QWidget *parent);

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

    void apply(vsg::PointerEvent& pointerEvent) override;

    void intersection(vsg::PointerEvent& pointerEvent);

private slots:
    void add();
    void start();
    void up();
    void down();
    void addBase();
    void reset();
    void typeChanged();

protected:
    void processSelection();

    vsg::ref_ptr<vsg::PointerEvent> lastPointerEvent;
    vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> lastIntersection;

    vsg::Intersector::NodePath::iterator _curr;

    enum Type
    {
        Move,
        Rot,
        Light,
        Alpha
    };

    Type _type;

    Ui::IntersectionHandler *ui;
    vsg::ref_ptr<Animation> _animation;
    vsg::ref_ptr<vsg::Group> _scenegraph;

    AnimationModel *_model;

    vsg::ref_ptr<vsg::MatrixTransform> _selected;
};



#endif // INTERSECTIONHANDLER_H
