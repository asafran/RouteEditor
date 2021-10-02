#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <QObject>
#include <vsg/all.h>
#include "SceneModel.h"
#include <QItemSelectionModel>

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::Group> in_scenegraph,
                double in_scale,
                vsg::ref_ptr<vsg::Options> in_options, QObject *parent = nullptr);

    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

    void apply(vsg::PointerEvent& pointerEvent) override;

    vsg::LineSegmentIntersector::Intersection interesection(vsg::PointerEvent& pointerEvent);
    enum Mode
    {
        SELECT,
        ADD,
        MOVE
    };
/*
public slots:
    void selectGroup(const QItemSelection &selected, const QItemSelection &);
    void selectNewObject(const QItemSelection &selected, const QItemSelection &);
*/
signals:
    void tileClicked();
    void addRequest(const vsg::dvec3 &pos);
    void objectClicked(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
    void sendCommand(QUndoCommand *command);

protected:
    inline void addPointer();

    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Group> scenegraph;
    vsg::ref_ptr<vsg::MatrixTransform> pointer;
    double height = 0.01;
    int mode = ADD;

    vsg::LineSegmentIntersector::Intersections lastIntersection;
};

#endif // MANIPULATOR_H
