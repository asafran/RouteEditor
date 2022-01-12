#ifndef OBJECTPROPERTIESEDITOR_H
#define OBJECTPROPERTIESEDITOR_H

#include "tool.h"
#include <QItemSelectionModel>
#include <vsg/viewer/EllipsoidModel.h>

namespace Ui {
class ObjectPropertiesEditor;
}

class ObjectPropertiesEditor : public Tool
{
    Q_OBJECT
public:
    explicit ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent = nullptr);
    virtual ~ObjectPropertiesEditor();

    //void addWireframe(const QModelIndex &index, const vsg::Node *node, vsg::dmat4 ltw);

    void intersection(const FindNode& isection) override;

public slots:
    void updateData();
    void selectObject(const QItemSelection &selected, const QItemSelection &deselected);
    void move(const vsg::dvec3 &delta);

signals:
    void objectClicked(const QModelIndex &index);
    void deselect();
    void deselectItem(const QModelIndex &index);
    void sendFirst(vsg::ref_ptr<route::SceneObject> firstObject);

private:

    void clear();
    void toggle(std::pair<const route::SceneObject*, const vsg::Node*> object);
    void select(const QModelIndex &index, route::SceneObject *object);

    Ui::ObjectPropertiesEditor *ui;

    vsg::ref_ptr<vsg::EllipsoidModel> _ellipsoidModel;

    vsg::ref_ptr<route::SceneObject> _firstObject;

    std::map<QModelIndex, route::SceneObject*> _selectedObjects;

    bool _single = true;

    double _xrot;
    double _yrot;
    double _zrot;
};

#endif // OBJECTPROPERTIESEDITOR_H
