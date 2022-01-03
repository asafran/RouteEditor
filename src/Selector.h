#ifndef SELECTOR_H
#define SELECTOR_H

#include <QObject>
#include <vsg/nodes/MatrixTransform.h>
#include "SceneModel.h"

class Selector : public QObject, public vsg::Inherit<vsg::Node, Selector>
{
    Q_OBJECT
public:
    Selector(vsg::ref_ptr<vsg::Group> root, vsg::ref_ptr<vsg::Builder> builder, QObject *parent = nullptr);

    ~Selector();

    template<class N, class V>
    static void t_traverse(N& node, V& visitor)
    {
        for (auto& frame : node._wireframes) frame.second->accept(visitor);
    }

    void traverse(vsg::Visitor& visitor) override { t_traverse(*this, visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { t_traverse(*this, visitor); }

public slots:
    void selectObject(const QItemSelection &selected, const QItemSelection &deselected);

signals:
    void objectSelected(vsg::ref_ptr<route::SceneObject> object);

private:
    //inline void createWireframe();

    vsg::ref_ptr<vsg::Builder> _builder;

    //vsg::ref_ptr<vsg::Node> _wireframe;

    vsg::ref_ptr<vsg::Group> _root;


    vsg::ref_ptr<route::Selection> _mergedSelection;
};

#endif // SELECTOR_H
