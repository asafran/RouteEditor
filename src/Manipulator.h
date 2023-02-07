#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <vsg/app/Trackball.h>
#include <QObject>
#include <vsg/nodes/Switch.h>
#include "SceneObjectVisitor.h"
#include <optional>

class DatabaseManager;

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                DatabaseManager *manager,
                QObject *parent = nullptr);
    ~Manipulator();

    void apply(vsg::KeyPressEvent& keyPress) override;
    void apply(vsg::KeyReleaseEvent& keyPress) override;
    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
    void apply(vsg::MoveEvent& pointerEvent) override;

    void rotate(double angle, const vsg::dvec3& axis) override;
    void zoom(double ratio) override;
    void pan(const vsg::dvec2& delta) override;

    FindNode intersectedObjects(const vsg::PointerEvent &pointerEvent);

    FindNode intersectedObjects(vsg::LineSegmentIntersector::Intersections isections);

public slots:
    void moveToObject(const QModelIndex &index);
    void setFirst(vsg::ref_ptr<route::SceneObject> firstObject);
    void startMoving();
    void setViewpoint(const vsg::dvec3 &pos);
    void setLatLongAlt(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);

signals:
    void sendPos(const vsg::dvec3 &pos);
    void sendMovingDelta(const vsg::dvec3 &delta);
    void sendIntersection(const FoundNodes& isection);
    //void objectClicked(const QModelIndex &index);
    void sendStatusText(const QString &message, int timeout);

protected:
    inline void createPointer();

    DatabaseManager *_database;

    vsg::ref_ptr<vsg::MatrixTransform> _pointer;

    bool _isMoving;
    vsg::ref_ptr<route::SceneObject> _movingObject;

    uint16_t _keyModifier = 0x0;

    vsg::LineSegmentIntersector::Intersection _lastIntersection;
};


#endif // MANIPULATOR_H
