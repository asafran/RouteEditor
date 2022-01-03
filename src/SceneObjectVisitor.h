#ifndef SCENEOBJECTVISITOR_H
#define SCENEOBJECTVISITOR_H

#include <vsg/core/Visitor.h>
#include <vsg/core/ConstVisitor.h>
#include <vsg/traversals/LineSegmentIntersector.h>

#include <QUndoStack>
#include "SceneModel.h"
#include <vsg/ui/KeyEvent.h>

namespace route {
    class SceneTrajectory;
    class SceneObject;
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
    class FindNode : public vsg::ConstVisitor, public vsg::LineSegmentIntersector::Intersection
    {
    public:
        FindNode(const vsg::LineSegmentIntersector::Intersection &lsi);
        FindNode();

        std::vector<std::pair<const route::SceneObject*, const vsg::Node*>> objects;
        std::pair<const route::Trajectory*, const vsg::Node*> track;
        std::pair<const route::SplinePoint*, const vsg::Node*> trackpoint;
        std::pair<const vsg::Switch*, const vsg::Node*> tile;

        vsg::KeyModifier keyModifier;

        void apply(const vsg::Node &node) override;

        void apply(const route::SceneObject &object);

        void apply(const route::Trajectory &traj);

        void apply(const route::SplinePoint &point);

        void apply(const vsg::Switch &sw) override;

    private:
        const vsg::Node *prev = nullptr;
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
