#include "SceneObjectVisitor.h"

#include "sceneobjects.h"
#include "trajectory.h"
#include "undo-redo.h"
#include <vsg/nodes/Switch.h>

/*
    SceneObjectsVisitor::SceneObjectsVisitor() : vsg::Visitor() {}

    void SceneObjectsVisitor::apply(SceneTrajectory& traj)
    {
        vsg::Visitor::apply(static_cast<vsg::Node&>(traj));
    }

    void SceneObjectsVisitor::apply(SceneObject& object)
    {
        vsg::Visitor::apply(static_cast<vsg::Node&>(object));
    }

    //----------------------------------------------------------------------------------------------------
    CSceneObjectsVisitor::CSceneObjectsVisitor() : vsg::ConstVisitor() {}

    void CSceneObjectsVisitor::apply(const SceneTrajectory& traj)
    {
        vsg::ConstVisitor::apply(static_cast<const vsg::Node&>(traj));
    }

    void CSceneObjectsVisitor::apply(const SceneObject& object)
    {
        vsg::ConstVisitor::apply(static_cast<const vsg::Node&>(object));
    }

    //----------------------------------------------------------------------------------------------------
    SceneObjectsFunctionVisitor::SceneObjectsFunctionVisitor(std::function<void(SceneTrajectory&)> trajF,
                        std::function<void(SceneObject&)> objectF)
        : SceneObjectsVisitor()
        , _trajFunction(trajF)
        , _objectFunction(objectF)
    {}

    void SceneObjectsFunctionVisitor::apply(SceneTrajectory& traj)
    {
        if (_trajFunction)
            _trajFunction(traj);
    }

    void SceneObjectsFunctionVisitor::apply(SceneObject& object)
    {
        if (_objectFunction)
            _objectFunction(object);
    }

    //----------------------------------------------------------------------------------------------------
    CSceneObjectsFunctionVisitor::CSceneObjectsFunctionVisitor(std::function<void(const SceneTrajectory&)> trajF,
                        std::function<void(const SceneObject&)> objectF)
        : CSceneObjectsVisitor()
        , _trajFunction(trajF)
        , _objectFunction(objectF)
    {}

    void CSceneObjectsFunctionVisitor::apply(const SceneTrajectory& traj)
    {
        if (_trajFunction)
            _trajFunction(traj);
    }

    void CSceneObjectsFunctionVisitor::apply(const SceneObject& object)
    {
        if (_objectFunction)
            _objectFunction(object);
    }
*/
    CalculateTransform::CalculateTransform() : vsg::Visitor()
    {
    }

    void CalculateTransform::apply(vsg::Node &node)
    {
        node.traverse(*this);
    }

    void CalculateTransform::apply(route::SceneObject &object)
    {
        auto newWorld = vsg::inverse(stack.top());
        auto wposition = object.getWorldPosition();
        undoStack->push(new ApplyTransformCalculation(&object, newWorld * wposition, newWorld));
    }

    void CalculateTransform::apply(vsg::Transform &transform)
    {
        if(auto object = transform.cast<route::SceneObject>(); object)
            apply(*object);
        stack.push(transform);
        transform.traverse(*this);
        stack.pop();
    }


    //----------------------------------------------------------------------------------------------------
    FindNode::FindNode(const vsg::LineSegmentIntersector::Intersection &lsi)
        : vsg::ConstVisitor()
        , vsg::LineSegmentIntersector::Intersection(lsi) {}

    FindNode::FindNode()
        : vsg::ConstVisitor()
        , vsg::LineSegmentIntersector::Intersection() {}

    void FindNode::apply(const vsg::Node &node)
    {
        if(auto object = node.cast<route::SceneObject>(); object)
            apply(*object);
        else if(auto traj = node.cast<route::Trajectory>(); traj)
            apply(*traj);
        else if(auto point = node.cast<route::SplinePoint>(); point)
            apply(*point);
        prev = &node;
    }

    void FindNode::apply(const route::SceneObject &object)
    {
        if(prev != nullptr)
            objects.push_back(std::make_pair(&object, prev));
        prev = &object;
    }

    void FindNode::apply(const route::Trajectory &traj)
    {
        if(prev != nullptr)
            track = std::make_pair(&traj, prev);
        prev = &traj;
    }

    void FindNode::apply(const route::SplinePoint &point)
    {
        if(prev != nullptr)
            trackpoint = std::make_pair(&point, prev);
        prev = &point;
    }

    void FindNode::apply(const vsg::Switch &sw)
    {
        if(prev != nullptr && sw.children.front().mask == route::Tiles)
            tile = std::make_pair(&sw, prev);
        prev = &sw;
    }
/*
CreateAddCommand::CreateAddCommand(const vsg::LineSegmentIntersector::Intersection &lsi,
                                   QUndoStack *stack, SceneModel *model)
    : vsg::ConstVisitor()
    , vsg::LineSegmentIntersector::Intersection(lsi)
    , _stack(stack)
    , _model(model) {}


void CreateAddCommand::apply(const vsg::Node &node)
{
    if(auto object = node.cast<route::SceneObject>(); object)
        apply(*object);
    else if(auto traj = node.cast<route::SceneTrajectory>(); traj)
        apply(*traj);
    _prev = &node;
}

void CreateAddCommand::apply(const route::SceneTrajectory &traj)
{

    auto coord = traj.traj->invert(worldIntersection);
    auto transform = traj.traj->createTransform(coord);

}

void CreateAddCommand::apply(const vsg::Switch &sw)
{
    if(_prev == nullptr || sw.children.front().mask == route::Tiles)
        return;



}*/
