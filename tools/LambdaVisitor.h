#ifndef CACHETILEVISITOR_H
#define CACHETILEVISITOR_H

#include <vsg/nodes/PagedLOD.h>

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

#endif // CACHETILEVISITOR_H
