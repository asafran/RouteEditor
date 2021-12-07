#ifndef CACHETILEVISITOR_H
#define CACHETILEVISITOR_H

#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/Switch.h>


class SceneObject;
class SceneTrajectory;

template<typename F1, typename C1>
class LambdaVisitor : public vsg::Visitor
{
public:
    LambdaVisitor(F1 func) :
        _function1(func) {}

    F1 _function1;

    using Visitor::apply;

    void apply(Object& object)
    {
        object.traverse(*this);
    }

    void apply(C1& object) override
    {
        _function1(object);
        object.traverse(*this);
    }
};

template<typename F1, typename C1>
class Lambda1Visitor : public vsg::Visitor
{
public:
    Lambda1Visitor(F1 func) :
        _function1(func) {}

    F1 _function1;

    using Visitor::apply;

    void apply(C1& object) override
    {
        _function1(object);
    }
};

template<typename F1, typename C1, typename F2, typename C2>
class Lambda2Visitor : public vsg::Visitor
{
public:
    Lambda2Visitor(F1 func1, F2 func2) :
        _function1(func1),
        _function2(func2) {}

    F1 _function1;
    F2 _function2;

    using vsg::Visitor::apply;

    void apply(C1& object) override
    {
        _function1(object);
        object.traverse(*this);
    }

    void apply(C2& object) override
    {
        _function2(object);
        object.traverse(*this);
    }
};

class SceneObjectsVisitor : public vsg::Visitor
{
public:
    SceneObjectsVisitor(std::function<void(SceneTrajectory&)> trajF = 0,
                        std::function<void(SceneObject&)> objectF = 0)
        : _trajFunction(trajF)
        , _objectFunction(objectF)
    {}

    std::function<void(SceneTrajectory&)> _trajFunction;
    std::function<void(SceneObject&)> _objectFunction;

    void apply(SceneTrajectory& traj)
    {
        if (_trajFunction)
            _trajFunction(traj);
    }

    void apply(SceneObject& object)
    {
        if (_objectFunction)
            _objectFunction(object);
    }
};
class ConstSceneObjectsVisitor : public vsg::ConstVisitor
{
public:
    ConstSceneObjectsVisitor(std::function<void(const SceneTrajectory&)> trajF = 0,
                        std::function<void(const SceneObject&)> objectF = 0)
        : _trajFunction(trajF)
        , _objectFunction(objectF)
    {}

    std::function<void(const SceneTrajectory&)> _trajFunction;
    std::function<void(const SceneObject&)> _objectFunction;

    virtual void apply(const SceneTrajectory& traj)
    {
        if (_trajFunction)
            _trajFunction(traj);
    }

    virtual void apply(const SceneObject& object)
    {
        if (_objectFunction)
            _objectFunction(object);
    }
};

template<typename F1, typename F2, typename F3> //template for auto& lambda
class FunctionVisitor : public SceneObjectsVisitor
{
public:
    FunctionVisitor(F1 autoF, F2 groupF, F3 plodF,
                    std::function<void(SceneTrajectory&)> trajF = 0,
                    std::function<void(SceneObject&)> objectF = 0)
        : SceneObjectsVisitor(trajF, objectF)
        , _autoFunction(autoF)
        , _groupFunction(groupF)
        , _plodFunction(plodF)
    {}

    F1 _autoFunction;
    F2 _groupFunction;
    F3 _plodFunction;

    //using vsg::Visitor::apply;

    void apply(vsg::Group& group) override
    {
        _groupFunction(group);
    }

    void apply(vsg::PagedLOD& plod) override
    {
        _plodFunction(plod);
    }
    void apply(vsg::Switch& sw) override
    {
        _autoFunction(sw);
    }
    void apply(vsg::LOD& lod) override
    {
        _autoFunction(lod);
    }
};
template<typename F1> //template for auto& lambda
class CFunctionVisitor : public ConstSceneObjectsVisitor
{
public:
    CFunctionVisitor(F1 autoF,
                    std::function<void(const vsg::Group&)> groupF,
                    std::function<void(const SceneTrajectory&)> trajF = 0)
        : ConstSceneObjectsVisitor(trajF)
        , _autoFunction(autoF)
        , _groupFunction(groupF)
    {}

    F1 _autoFunction;
    std::function<void(const vsg::Group&)> _groupFunction;

    //using vsg::Visitor::apply;

    void apply(const vsg::Group& group) override
    {
        if(_groupFunction)
            _groupFunction(group);
    }

    void apply(const vsg::PagedLOD& plod) override
    {
        _autoFunction(plod);
    }
    void apply(const vsg::Switch& sw) override
    {
        _autoFunction(sw);
    }
    void apply(const vsg::LOD& lod) override
    {
        _autoFunction(lod);
    }
};

#endif // CACHETILEVISITOR_H
