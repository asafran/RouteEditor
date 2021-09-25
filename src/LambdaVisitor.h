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

    void apply(C1& object) override
    {
        _function1(object);
        object.traverse(*this);
    }
};

#endif // CACHETILEVISITOR_H
