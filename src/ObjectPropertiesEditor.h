#ifndef OBJECTPROPERTIESEDITOR_H
#define OBJECTPROPERTIESEDITOR_H

#include "tool.h"
#include "stmodels.h"
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

    void intersection(const FoundNodes& isection) override;

public slots:
    void updateData();
    void clearSelection();
    void selectIndex(const QItemSelection &selected, const QItemSelection &deselected);
    void move(const vsg::dvec3 &delta);
    void selectObject(route::SceneObject *object);

    void updateRotation(double);

signals:
    void objectClicked(const QModelIndex &index);
    void deselect();
    void deselectItem(const QModelIndex &index);
    void sendFirst(vsg::ref_ptr<route::SceneObject> firstObject);

private:
    void clear();
    void toggle(route::SceneObject* object);
    void select(const QModelIndex &index, route::SceneObject *object);
    void setSpinEanbled(bool enabled);

    Ui::ObjectPropertiesEditor *ui;

    vsg::ref_ptr<vsg::EllipsoidModel> _ellipsoidModel;

    vsg::ref_ptr<route::SceneObject> _firstObject;

    std::map<QModelIndex, route::SceneObject*> _selectedObjects;

    bool _single = true;

    std::map<std::string, vsg::ref_ptr<signalling::Station>>::iterator _idx;
};

#endif // OBJECTPROPERTIESEDITOR_H
