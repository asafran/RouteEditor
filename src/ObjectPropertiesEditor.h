#ifndef OBJECTPROPERTIESEDITOR_H
#define OBJECTPROPERTIESEDITOR_H

#include <QWidget>
#include "TrajectoryModel.h"
#include <vsg/viewer/EllipsoidModel.h>

namespace Ui {
class ObjectPropertiesEditor;
}

class ObjectPropertiesEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectPropertiesEditor(vsg::ref_ptr<vsg::EllipsoidModel> model, QUndoStack *stack, QWidget *parent = nullptr);
    virtual ~ObjectPropertiesEditor();

public slots:
    void receiveObject(vsg::ref_ptr<route::SceneObject> object);
    void updateData();

private:
    Ui::ObjectPropertiesEditor *ui;

    vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel;
    QUndoStack *undoStack;

    vsg::ref_ptr<route::SceneObject> selectedObject;

    double xrot;
    double yrot;
    double zrot;
};

#endif // OBJECTPROPERTIESEDITOR_H
