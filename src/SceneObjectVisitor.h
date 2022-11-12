#ifndef SCENEOBJECTVISITOR_H
#define SCENEOBJECTVISITOR_H

#include <vsg/core/Visitor.h>
#include <vsg/core/ConstVisitor.h>
#include <vsg/traversals/LineSegmentIntersector.h>

#include <QUndoStack>
#include <vsg/ui/KeyEvent.h>
#include <vsg/vk/State.h>

namespace route {
    class SceneTrajectory;
    class SceneObject;
    class Trajectory;
    class RailPoint;
    class RailConnector;
}

/*
    class SceneObjectsVisitor : public vsg::Visitor
    {
    public:
        SceneObjectsVisitor();

        virtual void apply(SceneTrajectory& traj);

        virtual void apply(SceneObject& object);
    };

    class CSceneObjectsVisitor : public vsg::ConstVisitor
    {
    public:
        CSceneObjectsVisitor();

        virtual void apply(const SceneTrajectory& traj);

        virtual void apply(const SceneObject& object);
    };

    class SceneObjectsFunctionVisitor : public SceneObjectsVisitor
    {
    public:
        SceneObjectsFunctionVisitor(std::function<void(SceneTrajectory&)> trajF = 0,
                            std::function<void(SceneObject&)> objectF = 0);

        std::function<void(SceneTrajectory&)> _trajFunction;
        std::function<void(SceneObject&)> _objectFunction;

        void apply(SceneTrajectory& traj) override;

        void apply(SceneObject& object) override;
    };

    class CSceneObjectsFunctionVisitor : public CSceneObjectsVisitor
    {
    public:
        CSceneObjectsFunctionVisitor(std::function<void(const SceneTrajectory&)> trajF = 0,
                            std::function<void(const SceneObject&)> objectF = 0);

        std::function<void(const SceneTrajectory&)> _trajFunction;
        std::function<void(const SceneObject&)> _objectFunction;

        void apply(const SceneTrajectory& traj) override;

        void apply(const SceneObject& object) override;
    };

    template<typename F1, typename C1>
    class CSceneObjectsLambda1Visitor : public CSceneObjectsVisitor
    {
    public:
        CSceneObjectsLambda1Visitor(F1 func) :
            _function1(func) {}

        F1 _function1;

        using CSceneObjectsVisitor::apply;

        void apply(C1& object) override
        {
            _function1(object);
        }
    };
*/
    class CalculateTransform : public vsg::Visitor
    {
    public:
        CalculateTransform();

        vsg::MatrixStack stack;

        QUndoStack *undoStack = nullptr;

        void apply(vsg::Node &node) override;

        void apply(route::SceneObject &object);

        void apply(vsg::Transform &transform) override;
    };

    class ApplyTransform : public vsg::Visitor
    {
    public:
        ApplyTransform();

        std::stack<vsg::dmat4> stack;

        void apply(vsg::Node &node) override;

        void apply(route::SceneObject &object);

        void apply(vsg::Transform &transform) override;
    };

    class FoundNodes
    {
    public:
        FoundNodes(vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> lsi) : intersection(lsi)  {}
        FoundNodes() {}

        std::vector<route::SceneObject*> objects;
        route::Trajectory* trajectory = nullptr;
        route::RailPoint* trackpoint = nullptr;
        route::RailConnector* connector = nullptr;
        vsg::Switch* tile = nullptr;
        vsg::StateGroup* terrain = nullptr;

        vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> intersection;

        uint16_t keyModifier = 0;
    };

    class FindNode : public vsg::Visitor, public FoundNodes
    {
    public:
        FindNode(vsg::ref_ptr<vsg::LineSegmentIntersector::Intersection> lsi);
        FindNode();

        void apply(vsg::Node &node) override;

        void apply(vsg::StateGroup &group) override;

        void apply(vsg::Switch &sw) override;
    };
/*
    class CreateAddCommand : public vsg::ConstVisitor, public vsg::LineSegmentIntersector::Intersection
    {
    public:
        CreateAddCommand(const vsg::LineSegmentIntersector::Intersection &lsi,
                         QUndoStack *stack, SceneModel *model);

        void apply(const vsg::Node &node) override;

        //void apply(const route::SceneObject &object);

        void apply(const route::SceneTrajectory &traj);

        void apply(const vsg::Switch &sw) override;

        bool accepted = false;

    private:
        QUndoStack *_stack;
        SceneModel *_model;

        const vsg::Node *_prev = nullptr;
    };
*/


#endif // SCENEOBJECTVISITOR_H
