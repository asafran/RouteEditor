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
        if(undoStack)
            undoStack->push(new ApplyTransformCalculation(&object, newWorld * wposition, stack.top()));
        else
        {
            object.localToWorld = stack.top();
            object.setPosition(newWorld * wposition);
        }
        object.recalculateWireframe();
    }

    void CalculateTransform::apply(vsg::Transform &transform)
    {
        if(auto object = transform.cast<route::SceneObject>(); object)
            apply(*object);
        stack.push(transform);
        transform.traverse(*this);
        stack.pop();
    }

    ApplyTransform::ApplyTransform() : vsg::Visitor()
    {
    }

    void ApplyTransform::apply(vsg::Node &node)
    {
        node.traverse(*this);
    }

    void ApplyTransform::apply(route::SceneObject &object)
    {
        object.localToWorld = stack.top();
    }

    void ApplyTransform::apply(vsg::Transform &transform)
    {
        if(auto object = transform.cast<route::SceneObject>(); object)
            apply(*object);
        stack.push(transform.transform(stack.top()));
        transform.traverse(*this);
        stack.pop();
    }


    //----------------------------------------------------------------------------------------------------
    FindNode::FindNode(vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> lsi)
        : vsg::Visitor()
        , FoundNodes(lsi)
    {
        std::for_each(lsi->nodePath.begin(), lsi->nodePath.end(), [this](const vsg::Node *node) { const_cast<vsg::Node*>(node)->accept(*this); });
    }

    FindNode::FindNode()
        : vsg::Visitor()
        , FoundNodes() {}

    void FindNode::apply(vsg::Node &node)
    {
        if(auto object = node.cast<route::SceneObject>(); object)
            objects.push_back(object);
        if(auto traj = node.cast<route::Trajectory>(); traj)
            trajectory = traj;
        else if(auto conn = node.cast<route::RailConnector>(); conn)
            connector = conn;
        else if(auto point = node.cast<route::RailPoint>(); point)
            trackpoint = point;
    }

    void FindNode::apply(vsg::StateGroup &group)
    {
        vsg::Node *parent = nullptr;
        if(group.getValue(app::PARENT, parent) && parent == tile)
            terrain = &group;
    }

    void FindNode::apply(vsg::Switch &sw)
    {
        if(sw.children.front().mask == route::Tiles)
            tile = &sw;
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
