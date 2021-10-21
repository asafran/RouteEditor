#ifndef CACHETILEVISITOR_H
#define CACHETILEVISITOR_H

#include <vsg/nodes/PagedLOD.h>

template<typename F1, typename C1>
class LambdaVisitor : public vsg::ConstVisitor
{
public:
    LambdaVisitor(F1 func) :
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

#endif // CACHETILEVISITOR_H
