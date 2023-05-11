#ifndef OBJECTPROPERTIESEDITOR_H
#define OBJECTPROPERTIESEDITOR_H

#include "tool.h"
#include <unordered_set>
#include <QItemSelectionModel>
#include <vsg/app/EllipsoidModel.h>

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

public slots:
    void updateData();
    void clearSelection();
    void selectIndex(const QItemSelection &selected, const QItemSelection &deselected);
    void applyTransform(const vsg::dvec3 &delta);
    void selectObject(route::SceneObject *object);

    void updatePositionECEF(double);
    void updatePositionLLA(double);

    void updateRotation(double);

signals:
    void objectClicked(const QModelIndex &index);
    void deselect();
    void deselectItem(const QModelIndex &index);
    //void sendFirst(vsg::ref_ptr<route::SceneObject> firstObject);

private:
    void clear();
    void toggle(route::SceneObject* object);
    void setSpinEanbled(bool enabled);

    Ui::ObjectPropertiesEditor *ui;

    vsg::ref_ptr<vsg::EllipsoidModel> _ellipsoidModel;

    QSet<QModelIndex> _selectedObjects;

    bool _isSingle = true;
    bool _isShift = false;
    bool _isMoving = false;
    bool _isMpressed = false;

    vsg::dvec3 _prevIsection = {};

    // Visitor interface
public:
    void apply(vsg::KeyPressEvent &press) override;
    void apply(vsg::KeyReleaseEvent &release) override;
    void apply(vsg::ButtonPressEvent &) override;
    void apply(vsg::ButtonReleaseEvent &) override;
    void apply(vsg::MoveEvent &pointerEvent) override;
};

#endif // OBJECTPROPERTIESEDITOR_H
