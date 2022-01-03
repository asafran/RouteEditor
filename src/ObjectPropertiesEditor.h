#ifndef OBJECTPROPERTIESEDITOR_H
#define OBJECTPROPERTIESEDITOR_H

#include "tool.h"
#include "TrajectoryModel.h"
#include <vsg/viewer/EllipsoidModel.h>

namespace Ui {
class ObjectPropertiesEditor;
}

class ObjectPropertiesEditor : public Tool, public vsg::Inherit<vsg::Node, ObjectPropertiesEditor>
{
    Q_OBJECT
public:
    explicit ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent = nullptr);
    virtual ~ObjectPropertiesEditor();

    using vsg::Inherit<vsg::Node, ObjectPropertiesEditor>::create;

    void addWireframe(const QModelIndex &index, const vsg::Node *node, vsg::dmat4 ltw);

    void intersection(const FindNode& isection) override;

    template<class N, class V>
    static void t_traverse(N& node, V& visitor)
    {
        for (auto& frame : node._wireframes) frame.second->accept(visitor);
    }

    void traverse(vsg::Visitor& visitor) override { t_traverse(*this, visitor); }
    void traverse(vsg::ConstVisitor& visitor) const override { t_traverse(*this, visitor); }
    void traverse(vsg::RecordTraversal& visitor) const override { t_traverse(*this, visitor); }

public slots:
    void updateData();
    void selectObject(const QItemSelection &selected, const QItemSelection &deselected);

private:
    void clear();
    void select(std::pair<const route::SceneObject*, const vsg::Node*> object, const vsg::dmat4 &ltw);
    void merge(route::SceneObject *object);

    Ui::ObjectPropertiesEditor *ui;

    vsg::ref_ptr<vsg::EllipsoidModel> _ellipsoidModel;

    vsg::ref_ptr<route::SceneObject> _selectedObject;
    vsg::ref_ptr<route::Selection> _selectedObjects;

    std::map<QModelIndex, vsg::ref_ptr<vsg::MatrixTransform>> _wireframes;

    bool _single = true;

    double _xrot;
    double _yrot;
    double _zrot;
};

#endif // OBJECTPROPERTIESEDITOR_H
