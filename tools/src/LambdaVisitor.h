#ifndef CACHETILEVISITOR_H
#define CACHETILEVISITOR_H

#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/Switch.h>

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
class CLambdaVisitor : public vsg::ConstVisitor
{
public:
    CLambdaVisitor(F1 func) :
        _function1(func) {}

    F1 _function1;

    using ConstVisitor::apply;

    void apply(const Object& object)
    {
        object.traverse(*this);
    }

    void apply(const C1& object) override
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

template<typename F1, typename C1>
class CLambda1Visitor : public vsg::ConstVisitor
{
public:
    CLambda1Visitor(F1 func) :
        _function1(func) {}

    F1 _function1;

    using ConstVisitor::apply;

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

//template<typename F1> //template for auto& lambda
class FunctionVisitor : public vsg::Visitor
{
public:
    explicit FunctionVisitor(
                    std::function<void(vsg::Group&)> groupF,
                    std::function<void(vsg::Switch&)> swF = 0,
                    std::function<void(vsg::LOD&)> lodF = 0)
                    //std::function<void(SectionTrajectory&)> trajF = 0,
                    //std::function<void(SceneObject&)> objectF = 0)
        //: SceneObjectsVisitor(trajF, objectF)
        : vsg::Visitor()
        , _groupFunction(groupF)
        , _swFunction(swF)
        , _lodFunction(lodF)
    {}
    FunctionVisitor() : vsg::Visitor() {}

    std::function<void(vsg::Group&)> _groupFunction;
    std::function<void(vsg::Switch&)> _swFunction;
    std::function<void(vsg::LOD&)> _lodFunction;

    //using vsg::Visitor::apply;

    void apply(vsg::Group& group) override
    {
        if(_groupFunction)
            _groupFunction(group);
    }
    void apply(vsg::Switch& sw) override
    {
        if(_swFunction)
            _swFunction(sw);
        //else
           //Visitor::apply(static_cast<vsg::Node&>(sw));
    }
    void apply(vsg::LOD& lod) override
    {
        if(_lodFunction)
            _lodFunction(lod);
    }
};
template<typename F1> //template for auto& lambda
class CFunctionVisitor : public vsg::ConstVisitor//: public ConstSceneObjectsVisitor
{
public:
    CFunctionVisitor(F1 autoF)
                    //std::function<void(const SectionTrajectory&)> trajF = 0)
        //: ConstSceneObjectsVisitor(trajF)
        : vsg::ConstVisitor()
        , _autoFunction(autoF)
        //, _groupFunction(groupF)
    {}

    F1 _autoFunction;
    std::function<void(const vsg::Group&)> groupFunction;
    std::function<void(const vsg::Switch&)> swFunction;
    std::function<void(const vsg::PagedLOD&)> plodFunction;

    //using vsg::Visitor::apply;

    void apply(const vsg::Group& group) override
    {
        if(groupFunction)
            groupFunction(group);
    }

    void apply(const vsg::PagedLOD& plod) override
    {
        if(plodFunction)
            plodFunction(plod);
        else
            _autoFunction(plod);
    }
    void apply(const vsg::Switch& sw) override
    {
        if(swFunction)
            swFunction(sw);
        else
            _autoFunction(sw);
    }
    void apply(const vsg::LOD& lod) override
    {
        _autoFunction(lod);
    }
};


#endif // CACHETILEVISITOR_H
